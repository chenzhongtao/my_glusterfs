/*
  Copyright (c) 2008-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif

#include "io-cache.h"
#include "ioc-mem-types.h"

extern int ioc_log2_page_size;

/*
 * str_to_ptr - convert a string to pointer
 * @string: string
 *
 */
//字符串转地址
void *
str_to_ptr (char *string)
{
        void *ptr = NULL;

        GF_VALIDATE_OR_GOTO ("io-cache", string, out);

        ptr = (void *)strtoul (string, NULL, 16);

out:
        return ptr;
}


/*
 * ptr_to_str - convert a pointer to string
 * @ptr: pointer
 *
 */
// 地址转字符串
char *
ptr_to_str (void *ptr)
{
        int   ret = 0;
        char *str = NULL;

        GF_VALIDATE_OR_GOTO ("io-cache", ptr, out);

        ret = gf_asprintf (&str, "%p", ptr);
        if (-1 == ret) {
                gf_log ("io-cache", GF_LOG_WARNING,
                        "asprintf failed while converting ptr to str");
                str = NULL;
                goto out;
        }

out:
        return str;
}


// 唤醒inode waiter
void
ioc_inode_wakeup (call_frame_t *frame, ioc_inode_t *ioc_inode,
                  struct iatt *stbuf)
{
        ioc_waitq_t *waiter            = NULL, *waited = NULL;
        ioc_waitq_t *page_waitq        = NULL;
        int8_t       cache_still_valid = 1;
        ioc_local_t *local             = NULL;
        int8_t       need_fault        = 0;
        ioc_page_t  *waiter_page       = NULL;

        GF_VALIDATE_OR_GOTO ("io-cache", frame, out);

        local = frame->local;
        GF_VALIDATE_OR_GOTO (frame->this->name, local, out);

        if (ioc_inode == NULL) {
                local->op_ret = -1;
                local->op_errno = EINVAL;
                gf_log (frame->this->name, GF_LOG_WARNING, "ioc_inode is NULL");
                goto out;
        }

        ioc_inode_lock (ioc_inode);
        {
                waiter = ioc_inode->waitq;
                ioc_inode->waitq = NULL;
        }
        ioc_inode_unlock (ioc_inode);

        if (stbuf)
                cache_still_valid = ioc_cache_still_valid (ioc_inode, stbuf);
        else
                //缓存已失效
                cache_still_valid = 0;

        if (!waiter) {
                gf_log (frame->this->name, GF_LOG_WARNING,
                        "cache validate called without any "
                        "page waiting to be validated");
        }

        while (waiter) {
                // waiter中保存的页
                waiter_page = waiter->data;
                page_waitq = NULL;

                if (waiter_page) {
                        // 缓存仍有效，直接唤醒page
                        if (cache_still_valid) {
                                /* cache valid, wake up page */
                                ioc_inode_lock (ioc_inode);
                                {
                                        page_waitq =
                                                __ioc_page_wakeup (waiter_page,
                                                                   waiter_page->op_errno);
                                }
                                ioc_inode_unlock (ioc_inode);
                                if (page_waitq)
                                        ioc_waitq_return (page_waitq);
                        } else {
                                /* cache invalid, generate page fault and set
                                 * page->ready = 0, to avoid double faults
                                 设置 page->ready =0 ，避免两次触发
                                 */
                                ioc_inode_lock (ioc_inode);
                                {
                                        if (waiter_page->ready) {
                                                waiter_page->ready = 0;
                                                need_fault = 1; //需要触发页错误 
                                        } else {
                                                gf_log (frame->this->name,
                                                        GF_LOG_TRACE,
                                                        "validate frame(%p) is "
                                                        "waiting for in-transit"
                                                        " page = %p", frame,
                                                        waiter_page);
                                        }
                                }
                                ioc_inode_unlock (ioc_inode);

                                if (need_fault) {
                                        need_fault = 0;
                                        ioc_page_fault (ioc_inode, frame,
                                                        local->fd,
                                                        waiter_page->offset);
                                }
                        }
                }

                // 下一个waiter
                waited = waiter;
                waiter = waiter->next;

                waited->data = NULL;
                GF_FREE (waited);
        }

out:
        return;
}


/*
 * ioc_inode_update - create a new ioc_inode_t structure and add it to
 *                    the table table. fill in the fields which are derived
 *                    from inode_t corresponding to the file
 *
 * @table: io-table structure
 * @inode: inode structure
 *
 * not for external reference
 */

// 创建一个新的ioc_inode，并把它加入到table中
ioc_inode_t *
ioc_inode_update (ioc_table_t *table, inode_t *inode, uint32_t weight)
{
        ioc_inode_t     *ioc_inode   = NULL;

        GF_VALIDATE_OR_GOTO ("io-cache", table, out);

        ioc_inode = GF_CALLOC (1, sizeof (ioc_inode_t), gf_ioc_mt_ioc_inode_t);
        if (ioc_inode == NULL) {
                goto out;
        }
        //初始化ioc_indoe
        ioc_inode->inode = inode;
        ioc_inode->table = table;
        INIT_LIST_HEAD (&ioc_inode->cache.page_lru);
        pthread_mutex_init (&ioc_inode->inode_lock, NULL);
        ioc_inode->weight = weight;

        ioc_table_lock (table);
        {       
                // 添加到table中
                table->inode_count++;
                // 头插法，添加到table的inodes链表，所有ioc_inode都添加到这里
                list_add (&ioc_inode->inode_list, &table->inodes);
                // 尾插法，按优先级添加到inode_lru[weight]中
                list_add_tail (&ioc_inode->inode_lru,
                               &table->inode_lru[weight]);
        }
        ioc_table_unlock (table);

        gf_log (table->xl->name, GF_LOG_TRACE,
                "adding to inode_lru[%d]", weight);

out:
        return ioc_inode;
}


/*
 * ioc_inode_destroy - destroy an ioc_inode_t object.
 *
 * @inode: inode to destroy
 *
 * to be called only from ioc_forget.
 */
 // 释放ioc_inode
void
ioc_inode_destroy (ioc_inode_t *ioc_inode)
{
        ioc_table_t *table = NULL;

        GF_VALIDATE_OR_GOTO ("io-cache", ioc_inode, out);

        table = ioc_inode->table;

        ioc_table_lock (table);
        {       // 从table中删除
                table->inode_count--;
                list_del (&ioc_inode->inode_list);
                list_del (&ioc_inode->inode_lru);
        }
        ioc_table_unlock (table);
        //缓存释放
        ioc_inode_flush (ioc_inode);
        // 释放page
        rbthash_table_destroy (ioc_inode->cache.page_table);

        pthread_mutex_destroy (&ioc_inode->inode_lock);
        GF_FREE (ioc_inode);
out:
        return;
}
