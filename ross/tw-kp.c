/*
 * ROSS: Rensselaer's Optimistic Simulation System.
 * Copyright (c) 1999-2003 Rensselaer Polytechnic Instutitute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by David Bauer,
 *      Dr. Christopher D.  Carothers, and Shawn Pearce of the
 *      Department of Computer Science at Rensselaer Polytechnic
 *      Institute.
 *
 * 4. Neither the name of the University nor of the developers may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 5. The use or inclusion of this software or its documentation in
 *    any commercial product or distribution of this software to any
 *    other party without specific, written prior permission is
 *    prohibited.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <ross.h>

#if 0
inline tw_kp *
tw_kp_next_onpe(tw_kp * last, tw_pe * pe)
{
	if (!last)
		return pe->kp_list;
	if (!last->next || last->pe != pe)
		return NULL;
	return last->next;
}
#endif

inline void
tw_kp_onpe(tw_kp * kp, tw_pe * pe)
{
	kp->pe = pe;
}

void
tw_kp_rollback_to(tw_kp * kp, tw_stime to)
{
        tw_event       *e;

        kp->s_rb_total++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_to %f, now = %f \n",
		kp->pe->id, kp->id, to, kp->last_time);
#endif

        while(kp->pevent_q.size && kp->pevent_q.head->recv_ts >= to)
        {
                e = tw_eventq_shift(&kp->pevent_q);

                /*
                 * rollback first
                 */
                tw_event_rollback(e);

                /*
                 * reset kp pointers
                 */
                if (kp->pevent_q.size == 0)
                {
                        kp->last_time = kp->pe->GVT;
                } else
                {
                        kp->last_time = kp->pevent_q.head->recv_ts;
                }

                /*
                 * place event back into priority queue
                 */
                tw_pq_enqueue(kp->pe->pq, e);
        }
}

void
tw_kp_rollback_event(tw_event * event)
{
        tw_event       *e = NULL;
        tw_kp          *kp;
        tw_pe          *pe;

        kp = event->dest_lp->kp;
        pe = kp->pe;

        kp->s_rb_total++;
	kp->s_rb_secondary++;

#if VERIFY_ROLLBACK
        printf("%d %d: rb_event: %f \n", pe->id, kp->id, event->recv_ts);

	if(!kp->pevent_q.size)
		tw_error(TW_LOC, "Attempting to rollback empty pevent_q!");
#endif

	e = tw_eventq_shift(&kp->pevent_q);
        while(e != event)
	{
                kp->last_time = kp->pevent_q.head->recv_ts;
		tw_event_rollback(e);
                tw_pq_enqueue(pe->pq, e);

		e = tw_eventq_shift(&kp->pevent_q);
        }

        tw_event_rollback(e);

        if (0 == kp->pevent_q.size)
                kp->last_time = kp->pe->GVT;
        else
                kp->last_time = kp->pevent_q.head->recv_ts;
}

void
tw_init_kps(tw_pe * me)
{
	tw_kp *prev_kp = NULL;
	tw_kpid i;

	for (i = 0; i < g_tw_nkp; i++)
	{
		tw_kp *kp = &g_tw_kp[i];
		if (kp->pe != me)
			continue;

#if 0
		if (prev_kp)
			prev_kp->next = kp;
		else
			me->kp_list = kp;
#endif

		kp->s_e_rbs = 0;
		kp->s_rb_total = 0;
		kp->s_rb_secondary = 0;
		prev_kp = kp;
	}
}

tw_fd
tw_kp_next_free_q(tw_kp * kp)
{

	unsigned int             i;

	for (i = 0; i < g_tw_memory_nqueues; i++)
		if (kp->queues[i].size == -1)
			return i;

	tw_error(TW_LOC, "No more free queues on kp %d \n", kp->id);

	return -1;
}

tw_fd
tw_kp_memory_init(tw_kp * kp, size_t n_mem, size_t d_sz, size_t mult)
{
#
	tw_memoryq	*q;

	int             fd;

	fd = tw_kp_next_free_q(kp);

	q = tw_kp_getqueue(kp, fd);

	q->start_size = n_mem;
	q->d_size = d_sz;
	q->grow = mult;

	tw_memory_allocate(q, sizeof(tw_memory), q->start_size);

	kp->s_mem_buffers_used = 0;

	/*
	 * setup the processed buffer queue for this memory init 
	 */
	q = tw_kp_getqueue(kp, fd + 1);

	q->start_size = 0;
	q->size = 0;
	q->head = q->tail = NULL;

	return fd;
}

int
tw_kp_fossil_memory(tw_kp * me)
{
	tw_memoryq	*q;

	tw_memory      *b;
	tw_memory      *tail;

	tw_memory      *last;

	tw_stime	GVT = me->pe->GVT;

	unsigned int             i;
	int             rv;
	int             cnt;

	rv = 0;

	for (i = 1; i < g_tw_memory_nqueues; i+=2)
	{
		q = &me->queues[i];

		if (q->head == NULL)
		{
#if VERIFY_PE_FC_MEM
			printf("%d no bufs in pmemory_q! \n", me->id);
#endif
			return 0;
		}

		tail = q->tail;

		if (tail->ts >= GVT)
			return 0;

		if (q->head->ts < GVT)
		{
#if VERIFY_PE_FC_MEM
			printf("%d: collecting all bufs! \n", me->id);
#endif

			tw_memoryq_push_list(tw_kp_getqueue(me, i - 1), q->head, q->tail,
							   q->size);

			q->head = q->tail = NULL;
			rv = q->size;
			q->size = 0;

			return rv;
		}

		/*
		 * Start direct search.
		 */
		last = NULL;
		cnt = 0;

		b = q->head;
		while (b->ts >= GVT)
		{
			last = b;
			cnt++;

			b = b->next;
		}

		rv = q->size - cnt;
		tw_memoryq_push_list(tw_kp_getqueue(me, i - 1), b, q->tail, rv);

		/* fix what remains of our pmemory_q */
		q->tail = last;
		q->tail->next = NULL;
		q->size = cnt;

#if VERIFY_PE_FC_MEM
		printf("%d: direct search yielded %d bufs! \n", me->id, rv);
#endif
	}

	return rv;
}