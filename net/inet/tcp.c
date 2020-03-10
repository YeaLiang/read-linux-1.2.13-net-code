/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Implementation of the Transmission Control Protocol(TCP).
 *
 * Version:	@(#)tcp.c	1.0.16	05/25/93
 *
 * Authors:	Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Mark Evans, <evansmp@uhura.aston.ac.uk>
 *		Corey Minyard <wf-rch!minyard@relay.EU.net>
 *		Florian La Roche, <flla@stud.uni-sb.de>
 *		Charles Hedrick, <hedrick@klinzhai.rutgers.edu>
 *		Linus Torvalds, <torvalds@cs.helsinki.fi>
 *		Alan Cox, <gw4pts@gw4pts.ampr.org>
 *		Matthew Dillon, <dillon@apollo.west.oic.com>
 *		Arnt Gulbrandsen, <agulbra@no.unit.nvg>
 *
 * Fixes:	
 *		Alan Cox	:	Numerous verify_area() calls
 *		Alan Cox	:	Set the ACK bit on a reset
 *		Alan Cox	:	Stopped it crashing if it closed while sk->inuse=1
 *					and was trying to connect (tcp_err()).
 *		Alan Cox	:	All icmp error handling was broken
 *					pointers passed where wrong and the
 *					socket was looked up backwards. Nobody
 *					tested any icmp error code obviously.
 *		Alan Cox	:	tcp_err() now handled properly. It wakes people
 *					on errors. select behaves and the icmp error race
 *					has gone by moving it into sock.c
 *		Alan Cox	:	tcp_reset() fixed to work for everything not just
 *					packets for unknown sockets.
 *		Alan Cox	:	tcp option processing.
 *		Alan Cox	:	Reset tweaked (still not 100%) [Had syn rule wrong]
 *		Herp Rosmanith  :	More reset fixes
 *		Alan Cox	:	No longer acks invalid rst frames. Acking
 *					any kind of RST is right out.
 *		Alan Cox	:	Sets an ignore me flag on an rst receive
 *					otherwise odd bits of prattle escape still
 *		Alan Cox	:	Fixed another acking RST frame bug. Should stop
 *					LAN workplace lockups.
 *		Alan Cox	: 	Some tidyups using the new skb list facilities
 *		Alan Cox	:	sk->keepopen now seems to work
 *		Alan Cox	:	Pulls options out correctly on accepts
 *		Alan Cox	:	Fixed assorted sk->rqueue->next errors
 *		Alan Cox	:	PSH doesn't end a TCP read. Switched a bit to skb ops.
 *		Alan Cox	:	Tidied tcp_data to avoid a potential nasty.
 *		Alan Cox	:	Added some better commenting, as the tcp is hard to follow
 *		Alan Cox	:	Removed incorrect check for 20 * psh
 *	Michael O'Reilly	:	ack < copied bug fix.
 *	Johannes Stille		:	Misc tcp fixes (not all in yet).
 *		Alan Cox	:	FIN with no memory -> CRASH
 *		Alan Cox	:	Added socket option proto entries. Also added awareness of them to accept.
 *		Alan Cox	:	Added TCP options (SOL_TCP)
 *		Alan Cox	:	Switched wakeup calls to callbacks, so the kernel can layer network sockets.
 *		Alan Cox	:	Use ip_tos/ip_ttl settings.
 *		Alan Cox	:	Handle FIN (more) properly (we hope).
 *		Alan Cox	:	RST frames sent on unsynchronised state ack error/
 *		Alan Cox	:	Put in missing check for SYN bit.
 *		Alan Cox	:	Added tcp_select_window() aka NET2E 
 *					window non shrink trick.
 *		Alan Cox	:	Added a couple of small NET2E timer fixes
 *		Charles Hedrick :	TCP fixes
 *		Toomas Tamm	:	TCP window fixes
 *		Alan Cox	:	Small URG fix to rlogin ^C ack fight
 *		Charles Hedrick	:	Rewrote most of it to actually work
 *		Linus		:	Rewrote tcp_read() and URG handling
 *					completely
 *		Gerhard Koerting:	Fixed some missing timer handling
 *		Matthew Dillon  :	Reworked TCP machine states as per RFC
 *		Gerhard Koerting:	PC/TCP workarounds
 *		Adam Caldwell	:	Assorted timer/timing errors
 *		Matthew Dillon	:	Fixed another RST bug
 *		Alan Cox	:	Move to kernel side addressing changes.
 *		Alan Cox	:	Beginning work on TCP fastpathing (not yet usable)
 *		Arnt Gulbrandsen:	Turbocharged tcp_check() routine.
 *		Alan Cox	:	TCP fast path debugging
 *		Alan Cox	:	Window clamping
 *		Michael Riepe	:	Bug in tcp_check()
 *		Matt Dillon	:	More TCP improvements and RST bug fixes
 *		Matt Dillon	:	Yet more small nasties remove from the TCP code
 *					(Be very nice to this man if tcp finally works 100%) 8)
 *		Alan Cox	:	BSD accept semantics. 
 *		Alan Cox	:	Reset on closedown bug.
 *	Peter De Schrijver	:	ENOTCONN check missing in tcp_sendto().
 *		Michael Pall	:	Handle select() after URG properly in all cases.
 *		Michael Pall	:	Undo the last fix in tcp_read_urg() (multi URG PUSH broke rlogin).
 *		Michael Pall	:	Fix the multi URG PUSH problem in tcp_readable(), select() after URG works now.
 *		Michael Pall	:	recv(...,MSG_OOB) never blocks in the BSD api.
 *		Alan Cox	:	Changed the semantics of sk->socket to 
 *					fix a race and a signal problem with
 *					accept() and async I/O.
 *		Alan Cox	:	Relaxed the rules on tcp_sendto().
 *		Yury Shevchuk	:	Really fixed accept() blocking problem.
 *		Craig I. Hagan  :	Allow for BSD compatible TIME_WAIT for
 *					clients/servers which listen in on
 *					fixed ports.
 *		Alan Cox	:	Cleaned the above up and shrank it to
 *					a sensible code size.
 *		Alan Cox	:	Self connect lockup fix.
 *		Alan Cox	:	No connect to multicast.
 *		Ross Biro	:	Close unaccepted children on master
 *					socket close.
 *		Alan Cox	:	Reset tracing code.
 *		Alan Cox	:	Spurious resets on shutdown.
 *		Alan Cox	:	Giant 15 minute/60 second timer error
 *		Alan Cox	:	Small whoops in selecting before an accept.
 *		Alan Cox	:	Kept the state trace facility since it's
 *					handy for debugging.
 *		Alan Cox	:	More reset handler fixes.
 *		Alan Cox	:	Started rewriting the code based on the RFC's
 *					for other useful protocol references see:  
 *					Comer, KA9Q NOS, and for a reference on the
 *					difference between specifications and how BSD
 *					works see the 4.4lite source.
 *		A.N.Kuznetsov	:	Don't time wait on completion of tidy 
 *					close.
 *		Linus Torvalds	:	Fin/Shutdown & copied_seq changes.
 *		Linus Torvalds	:	Fixed BSD port reuse to work first syn
 *		Alan Cox	:	Reimplemented timers as per the RFC and using multiple
 *					timers for sanity. 
 *		Alan Cox	:	Small bug fixes, and a lot of new
 *					comments.
 *		Alan Cox	:	Fixed dual reader crash by locking
 *					the buffers (much like datagram.c)
 *		Alan Cox	:	Fixed stuck sockets in probe. A probe
 *					now gets fed up of retrying without
 *					(even a no space) answer.
 *		Alan Cox	:	Extracted closing code better
 *		Alan Cox	:	Fixed the closing state machine to
 *					resemble the RFC.
 *		Alan Cox	:	More 'per spec' fixes.
 *		Alan Cox	:	tcp_data() doesn't ack illegal PSH
 *					only frames. At least one pc tcp stack
 *					generates them.
 *		Mark Yarvis	:	In tcp_read_wakeup(), don't send an
 *					ack if stat is TCP_CLOSED.
 *
 *
 * To Fix:
 *		Fast path the code. Two things here - fix the window calculation
 *		so it doesn't iterate over the queue, also spot packets with no funny
 *		options arriving in order and process directly.
 *
 *		Implement RFC 1191 [Path MTU discovery]
 *		Look at the effect of implementing RFC 1337 suggestions and their impact.
 *		Rewrite output state machine to use a single queue and do low window
 *		situations as per the spec (RFC 1122)
 *		Speed up input assembly algorithm.
 *		RFC1323 - PAWS and window scaling. PAWS is required for IPv6 so we
 *		could do with it working on IPv4
 *		User settable/learned rtt/max window/mtu
 *		Cope with MTU/device switches when retransmitting in tcp.
 *		Fix the window handling to use PR's new code.
 *
 *		Change the fundamental structure to a single send queue maintained
 *		by TCP (removing the bogus ip stuff [thus fixing mtu drops on
 *		active routes too]). Cut the queue off in tcp_retransmit/
 *		tcp_transmit.
 *		Change the receive queue to assemble as it goes. This lets us
 *		dispose of most of tcp_sequence, half of tcp_ack and chunks of
 *		tcp_data/tcp_read as well as the window shrink crud.
 *		Separate out duplicated code - tcp_alloc_skb, tcp_build_ack
 *		tcp_queue_skb seem obvious routines to extract.
 *	
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or(at your option) any later version.
 *
 * Description of States:
 *
 *	TCP_SYN_SENT		sent a connection request, waiting for ack
 *
 *	TCP_SYN_RECV		received a connection request, sent ack,
 *				waiting for final ack in three-way handshake.
 *
 *	TCP_ESTABLISHED		connection established
 *
 *	TCP_FIN_WAIT1		our side has shutdown, waiting to complete
 *				transmission of remaining buffered data
 *
 *	TCP_FIN_WAIT2		all buffered data sent, waiting for remote
 *				to shutdown
 *
 *	TCP_CLOSING		both sides have shutdown but we still have
 *				data we have to finish sending
 *
 *	TCP_TIME_WAIT		timeout to catch resent junk before entering
 *				closed, can only be entered from FIN_WAIT2
 *				or CLOSING.  Required because the other end
 *				may not have gotten our last ACK causing it
 *				to retransmit the data packet (which we ignore)
 *
 *	TCP_CLOSE_WAIT		remote side has shutdown and is waiting for
 *				us to finish writing our data and to shutdown
 *				(we have to close() to move on to LAST_ACK)
 *
 *	TCP_LAST_ACK		out side has shutdown after remote has
 *				shutdown.  There may still be data in our
 *				buffer that we have to finish sending
 *		
 *	TCP_CLOSE		socket is finished
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/config.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/termios.h>
#include <linux/in.h>
#include <linux/fcntl.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include "snmp.h"
#include "ip.h"
#include "protocol.h"
#include "icmp.h"
#include "tcp.h"
#include "arp.h"
#include <linux/skbuff.h>
#include "sock.h"
#include "route.h"
#include <linux/errno.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <linux/mm.h>

/*
 *	The MSL timer is the 'normal' timer.
 */
 
#define reset_msl_timer(x,y,z)	reset_timer(x,y,z)

#define SEQ_TICK 3
unsigned long seq_offset;
struct tcp_mib	tcp_statistics;

static void tcp_close(struct sock *sk, int timeout);


/*
 *	The less said about this the better, but it works and will do for 1.2 
 */
// 等待建立连接的进程队列
static struct wait_queue *master_select_wakeup;

static __inline__ int min(unsigned int a, unsigned int b)
{
	if (a < b) 
		return(a);
	return(b);
}

#undef STATE_TRACE

#ifdef STATE_TRACE
static char *statename[]={
	"Unused","Established","Syn Sent","Syn Recv",
	"Fin Wait 1","Fin Wait 2","Time Wait", "Close",
	"Close Wait","Last ACK","Listen","Closing"
};
#endif

static __inline__ void tcp_set_state(struct sock *sk, int state)
{
	if(sk->state==TCP_ESTABLISHED)
		tcp_statistics.TcpCurrEstab--;
#ifdef STATE_TRACE
	if(sk->debug)
		printk("TCP sk=%p, State %s -> %s\n",sk, statename[sk->state],statename[state]);
#endif	
	/* This is a hack but it doesn't occur often and it's going to
	   be a real        to fix nicely */
	// 新状态为连接建立，老状态为syn_recv，则说明有连接可用，唤醒等待连接的进程队列 
	if(state==TCP_ESTABLISHED && sk->state==TCP_SYN_RECV)
	{
		wake_up_interruptible(&master_select_wakeup);
	}
	sk->state=state;
	if(state==TCP_ESTABLISHED)
		tcp_statistics.TcpCurrEstab++;
}

/*
 *	This routine picks a TCP windows for a socket based on
 *	the following constraints
 *  
 *	1. The window can never be shrunk once it is offered (RFC 793)
 *	2. We limit memory per socket
 *   
 *	For now we use NET2E3's heuristic of offering half the memory
 *	we have handy. All is not as bad as this seems however because
 *	of two things. Firstly we will bin packets even within the window
 *	in order to get the data we are waiting for into the memory limit.
 *	Secondly we bin common duplicate forms at receive time
 *      Better heuristics welcome
 */
// 计算本端的接收窗口大小   
int tcp_select_window(struct sock *sk)
{	
	// 还有多少用于读的空间
	int new_window = sk->prot->rspace(sk);
	// 路由项中设定的窗口大小	
	if(sk->window_clamp)
		// 取小的
		new_window=min(sk->window_clamp,new_window);
	/*
	 * 	Two things are going on here.  First, we don't ever offer a
	 * 	window less than min(sk->mss, MAX_WINDOW/2).  This is the
	 * 	receiver side of SWS as specified in RFC1122.
	 * 	Second, we always give them at least the window they
	 * 	had before, in order to avoid retracting window.  This
	 * 	is technically allowed, but RFC1122 advises against it and
	 * 	in practice it causes trouble.
	 *
	 * 	Fixme: This doesn't correctly handle the case where
	 *	new_window > sk->window but not by enough to allow for the
	 *	shift in sequence space. 
	 */
	// 保证窗口不比之前的小，否则可能引起对端会传小数据包
	if (new_window < min(sk->mss, MAX_WINDOW/2) || new_window < sk->window)
		return(sk->window);
	return(new_window);
}

/*
 *	Find someone to 'accept'. Must be called with
 *	sk->inuse=1 or cli()
 */ 
// 找出已经完成三次握手的socket
static struct sk_buff *tcp_find_established(struct sock *s)
{
	struct sk_buff *p=skb_peek(&s->receive_queue);
	if(p==NULL)
		return NULL;
	do
	{
		if(p->sk->state == TCP_ESTABLISHED || p->sk->state >= TCP_FIN_WAIT1)
			return p;
		p=p->next;
	}
	while(p!=(struct sk_buff *)&s->receive_queue);
	return NULL;
}

/*
 *	Remove a completed connection and return it. This is used by
 *	tcp_accept() to get connections from the queue.
 */
// 返回一个完成的连接
static struct sk_buff *tcp_dequeue_established(struct sock *s)
{
	struct sk_buff *skb;
	unsigned long flags;
	save_flags(flags);
	cli(); 
	skb=tcp_find_established(s);
	if(skb!=NULL)
		skb_unlink(skb);	/* Take it off the queue */
	restore_flags(flags);
	return skb;
}

/* 
 *	This routine closes sockets which have been at least partially
 *	opened, but not yet accepted. Currently it is only called by
 *	tcp_close, and timeout mirrors the value there. 
 */
// 用于listen型的socket
static void tcp_close_pending (struct sock *sk) 
{
	struct sk_buff *skb;
	// 置socket为释放状态，关闭建立的连接，释放读缓冲区空间
	while ((skb = skb_dequeue(&sk->receive_queue)) != NULL) 
	{
		skb->sk->dead=1;
		tcp_close(skb->sk, 0);
		kfree_skb(skb, FREE_READ);
	}
	return;
}

/*
 *	Enter the time wait state. 
 */
// 进入time_wait状态，开启msl定时器
static void tcp_time_wait(struct sock *sk)
{
	tcp_set_state(sk,TCP_TIME_WAIT);
	sk->shutdown = SHUTDOWN_MASK;
	if (!sk->dead)
		sk->state_change(sk);
	reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
}

/*
 *	A socket has timed out on its send queue and wants to do a
 *	little retransmitting. Currently this means TCP.
 */
// 重传数据包，all等于0只重传一个，否则传多个
void tcp_do_retransmit(struct sock *sk, int all)
{
	struct sk_buff * skb;
	struct proto *prot;
	struct device *dev;
	int ct=0;

	prot = sk->prot;
	// 已发送还没收到ack的skb队列
	skb = sk->send_head;

	while (skb != NULL)
	{
		struct tcphdr *th;
		struct iphdr *iph;
		int size;

		dev = skb->dev;
		IS_SKB(skb);
		// 发送的开始时间
		skb->when = jiffies;

		/*
		 * In general it's OK just to use the old packet.  However we
		 * need to use the current ack and window fields.  Urg and
		 * urg_ptr could possibly stand to be updated as well, but we
		 * don't keep the necessary data.  That shouldn't be a problem,
		 * if the other end is doing the right thing.  Since we're
		 * changing the packet, we have to issue a new IP identifier.
		 */
		// 指向ip头部
		iph = (struct iphdr *)(skb->data + dev->hard_header_len);
		// 指向tcp头部
		th = (struct tcphdr *)(((char *)iph) + (iph->ihl << 2));
		// tcp头+数据部分的长度
		size = skb->len - (((unsigned char *) th) - skb->data);
		
		/*
		 *	Note: We ought to check for window limits here but
		 *	currently this is done (less efficiently) elsewhere.
		 *	We do need to check for a route change but can't handle
		 *	that until we have the new 1.3.x buffers in.
		 *
		 */
		// 重新取一个id
		iph->id = htons(ip_id_count++);
		// 计算校验和
		ip_send_check(iph);

		/*
		 *	This is not the right way to handle this. We have to
		 *	issue an up to date window and ack report with this 
		 *	retransmit to keep the odd buggy tcp that relies on 
		 *	the fact BSD does this happy. 
		 *	We don't however need to recalculate the entire 
		 *	checksum, so someone wanting a small problem to play
		 *	with might like to implement RFC1141/RFC1624 and speed
		 *	this up by avoiding a full checksum.
		 */
		// 当前的ack
		th->ack_seq = ntohl(sk->acked_seq);
		th->window = ntohs(tcp_select_window(sk));
		tcp_send_check(th, sk->saddr, sk->daddr, size, sk);
		
		/*
		 *	If the interface is (still) up and running, kick it.
		 */
		// 设备还在工作
		if (dev->flags & IFF_UP)
		{
			/*
			 *	If the packet is still being sent by the device/protocol
			 *	below then don't retransmit. This is both needed, and good -
			 *	especially with connected mode AX.25 where it stops resends
			 *	occurring of an as yet unsent anyway frame!
			 *	We still add up the counts as the round trip time wants
			 *	adjusting.
			 */
			if (sk && !skb_device_locked(skb))
			{
				/* Remove it from any existing driver queue first! */
				skb_unlink(skb);
				/* Now queue it */
				ip_statistics.IpOutRequests++;
				// 发送
				dev_queue_xmit(skb, dev, sk->priority);
			}
		}

		/*
		 *	Count retransmissions
		 */
		// 累积重传的数据包的个数，不能超过拥塞窗口大小		 
		ct++;

		sk->prot->retransmits ++;

		/*
		 *	Only one retransmit requested.
		 */
		// all等于0说明只传一个数据包就行
		if (!all)
			break;

		/*
		 *	This should cut it off before we send too many packets.
		 */
		// 超过了拥塞窗口则暂停发送
		if (ct >= sk->cong_window)
			break;
		// 指向下一个待发送的skb
		skb = skb->link3;
	}
}

/*
 *	Reset the retransmission timer
 */
// 重置重传定时器 
static void reset_xmit_timer(struct sock *sk, int why, unsigned long when)
{
	del_timer(&sk->retransmit_timer);
	sk->ip_xmit_timeout = why;
	if((int)when < 0)
	{
		when=3;
		printk("Error: Negative timer in xmit_timer\n");
	}
	sk->retransmit_timer.expires=when;
	add_timer(&sk->retransmit_timer);
}

/*
 * 	This is the normal code called for timeouts.  It does the retransmission
 * 	and then does backoff.  tcp_do_retransmit is separated out because
 * 	tcp_ack needs to send stuff from the retransmit queue without
 * 	initiating a backoff.
 */

// 重传，然后重置超时定时器,为rto
void tcp_retransmit_time(struct sock *sk, int all)
{
	tcp_do_retransmit(sk, all);

	/*
	 * Increase the timeout each time we retransmit.  Note that
	 * we do not increase the rtt estimate.  rto is initialized
	 * from rtt, but increases here.  Jacobson (SIGCOMM 88) suggests
	 * that doubling rto each time is the least we can get away with.
	 * In KA9Q, Karn uses this for the first few times, and then
	 * goes to quadratic.  netBSD doubles, but only goes up to *64,
	 * and clamps at 1 to 64 sec afterwards.  Note that 120 sec is
	 * defined in the protocol as the maximum possible RTT.  I guess
	 * we'll have to use something other than TCP to talk to the
	 * University of Mars.
	 *
	 * PAWS allows us longer timeouts and large windows, so once
	 * implemented ftp to mars will work nicely. We will have to fix
	 * the 120 second clamps though!
	 */
	// 重传次数累加
	sk->retransmits++;
	sk->backoff++;
	sk->rto = min(sk->rto << 1, 120*HZ);
	reset_xmit_timer(sk, TIME_WRITE, sk->rto);
}


/*
 *	A timer event has trigger a tcp retransmit timeout. The
 *	socket xmit queue is ready and set up to send. Because
 *	the ack receive code keeps the queue straight we do
 *	nothing clever here.
 */

static void tcp_retransmit(struct sock *sk, int all)
{
	if (all) 
	{
		tcp_retransmit_time(sk, all);
		return;
	}
	// 减少发送数据包的数量
	sk->ssthresh = sk->cong_window >> 1; /* remember window where we lost */
	/* sk->ssthresh in theory can be zero.  I guess that's OK */
	sk->cong_count = 0;
	// 先传一个数据包
	sk->cong_window = 1;

	/* Do the actual retransmit. */
	tcp_retransmit_time(sk, all);
}

/*
 *	A write timeout has occurred. Process the after effects.
 */

static int tcp_write_timeout(struct sock *sk)
{
	/*
	 *	Look for a 'soft' timeout.
	 */
	if ((sk->state == TCP_ESTABLISHED && sk->retransmits && !(sk->retransmits & 7))
		|| (sk->state != TCP_ESTABLISHED && sk->retransmits > TCP_RETR1)) 
	{
		/*
		 *	Attempt to recover if arp has changed (unlikely!) or
		 *	a route has shifted (not supported prior to 1.3).
		 */
		arp_destroy (sk->daddr, 0);
		ip_route_check (sk->daddr);
	}
	/*
	 *	Has it gone just too far ?
	 */
	// 重传次数超过限制
	if (sk->retransmits > TCP_RETR2) 
	{
		sk->err = ETIMEDOUT;
		sk->error_report(sk);
		del_timer(&sk->retransmit_timer);
		/*
		 *	Time wait the socket 
		 */
		// 认为socket已经断开连接，准备或直接置为关闭状态
		if (sk->state == TCP_FIN_WAIT1 || sk->state == TCP_FIN_WAIT2 || sk->state == TCP_CLOSING ) 
		{
			tcp_set_state(sk,TCP_TIME_WAIT);
			reset_msl_timer (sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
		}
		else
		{
			/*
			 *	Clean up time.
			 */
			tcp_set_state(sk, TCP_CLOSE);
			return 0;
		}
	}
	return 1;
}

/*
 *	The TCP retransmit timer. This lacks a few small details.
 *
 *	1. 	An initial rtt timeout on the probe0 should cause what we can
 *		of the first write queue buffer to be split and sent.
 *	2.	On a 'major timeout' as defined by RFC1122 we shouldn't report
 *		ETIMEDOUT if we know an additional 'soft' error caused this.
 *		tcp_err should save a 'soft error' for us.
 */

static void retransmit_timer(unsigned long data)
{
	struct sock *sk = (struct sock*)data;
	int why = sk->ip_xmit_timeout;

	/* 
	 * only process if socket is not in use
	 */

	cli();
	// socket正在被使用，一秒后重试
	if (sk->inuse || in_bh) 
	{
		/* Try again in 1 second */
		sk->retransmit_timer.expires = HZ;
		add_timer(&sk->retransmit_timer);
		sti();
		return;
	}

	sk->inuse = 1;
	sti();

	/* Always see if we need to send an ack. */
	// 缓存的未发送ack的包个数
	if (sk->ack_backlog && !sk->zapped) 
	{
		sk->prot->read_wakeup (sk);
		if (! sk->dead)
			sk->data_ready(sk,0);
	}

	/* Now we need to figure out why the socket was on the timer. */

	switch (why) 
	{
		/* Window probing */
		case TIME_PROBE0:
			// 发送探测窗口的数据包
			tcp_send_probe0(sk);
			// 是否超过了重试次数
			tcp_write_timeout(sk);
			break;
		/* Retransmitting */
		case TIME_WRITE:
			/* It could be we got here because we needed to send an ack.
			 * So we need to check for that.
			 */
		{
			struct sk_buff *skb;
			unsigned long flags;

			save_flags(flags);
			cli();
			skb = sk->send_head;
			if (!skb) 
			{
				restore_flags(flags);
			} 
			else 
			{
				/*
				 *	Kicked by a delayed ack. Reset timer
				 *	correctly now
				 */
				// 还没超时，重置定时器时间为剩下的超时时间
				if (jiffies < skb->when + sk->rto) 
				{
					reset_xmit_timer (sk, TIME_WRITE, skb->when + sk->rto - jiffies);
					restore_flags(flags);
					break;
				}
				restore_flags(flags);
				/*
				 *	Retransmission
				 */
				// 超时重传
				sk->prot->retransmit (sk, 0);
				tcp_write_timeout(sk);
			}
			break;
		}
		/* Sending Keepalives */
		case TIME_KEEPOPEN:
			/* 
			 * this reset_timer() call is a hack, this is not
			 * how KEEPOPEN is supposed to work.
			 */
			// 重置定时器，一定时间后继续发送保活数据包
			reset_xmit_timer (sk, TIME_KEEPOPEN, TCP_TIMEOUT_LEN);

			/* Send something to keep the connection open. */
			// 发送保活数据包
			if (sk->prot->write_wakeup)
				  sk->prot->write_wakeup (sk);
			sk->retransmits++;
			tcp_write_timeout(sk);
			break;
		default:
			printk ("rexmit_timer: timer expired - reason unknown\n");
			break;
	}
	release_sock(sk);
}

/*
 * This routine is called by the ICMP module when it gets some
 * sort of error condition.  If err < 0 then the socket should
 * be closed and the error returned to the user.  If err > 0
 * it's just the icmp type << 8 | icmp code.  After adjustment
 * header points to the first 8 bytes of the tcp header.  We need
 * to find the appropriate port.
 */
// 错误处理
void tcp_err(int err, unsigned char *header, unsigned long daddr,
	unsigned long saddr, struct inet_protocol *protocol)
{
	struct tcphdr *th;
	struct sock *sk;
	struct iphdr *iph=(struct iphdr *)header;
  
	header+=4*iph->ihl;
   

	th =(struct tcphdr *)header;
	sk = get_sock(&tcp_prot, th->source, daddr, th->dest, saddr);

	if (sk == NULL) 
		return;
  
	if(err<0)
	{
	  	sk->err = -err;
	  	sk->error_report(sk);
	  	return;
	}

	if ((err & 0xff00) == (ICMP_SOURCE_QUENCH << 8)) 
	{
		/*
		 * FIXME:
		 * For now we will just trigger a linear backoff.
		 * The slow start code should cause a real backoff here.
		 */
		if (sk->cong_window > 4)
			sk->cong_window--;
		return;
	}

/*	sk->err = icmp_err_convert[err & 0xff].errno;  -- moved as TCP should hide non fatals internally (and does) */

	/*
	 * If we've already connected we will keep trying
	 * until we time out, or the user gives up.
	 */

	if (icmp_err_convert[err & 0xff].fatal || sk->state == TCP_SYN_SENT) 
	{
		if (sk->state == TCP_SYN_SENT) 
		{
			tcp_statistics.TcpAttemptFails++;
			tcp_set_state(sk,TCP_CLOSE);
			sk->error_report(sk);		/* Wake people up to see the error (see connect in sock.c) */
		}
		sk->err = icmp_err_convert[err & 0xff].errno;		
	}
	return;
}


/*
 *	Walk down the receive queue counting readable data until we hit the end or we find a gap
 *	in the received data queue (ie a frame missing that needs sending to us). Not
 *	sorting using two queues as data arrives makes life so much harder.
 */
// 判断有多少可读的数据
static int tcp_readable(struct sock *sk)
{
	unsigned long counted;
	unsigned long amount;
	struct sk_buff *skb;
	int sum;
	unsigned long flags;

	if(sk && sk->debug)
	  	printk("tcp_readable: %p - ",sk);

	save_flags(flags);
	cli();
	if (sk == NULL || (skb = skb_peek(&sk->receive_queue)) == NULL)
	{
		restore_flags(flags);
	  	if(sk && sk->debug) 
	  		printk("empty\n");
	  	return(0);
	}
	// 应用程序可以读取但是还没读取的字节的序列号,即copied_seq之前的都已经读取
	counted = sk->copied_seq;	/* Where we are at the moment */
	amount = 0;
  
	/* 
	 *	Do until a push or until we are out of data. 
	 */
	 
	do 
	{	// 小于则退出，说明到达的包是乱序的，一般来说两者是等于
		if (before(counted, skb->h.th->seq)) 	/* Found a hole so stops here */
			break;
		// counted - skb->h.th->seq一般是等于0的，如果大于0说明有一部分数据已经读过了，就跳过这部分数据
		sum = skb->len -(counted - skb->h.th->seq);	/* Length - header but start from where we are up to (avoid overlaps) */
		// sum代表的是每一个skb中消耗的序列号，所以如果是syn包，skb->len是等于0，但序列号要加1。
		if (skb->h.th->syn)
			sum++;
		// 消耗了序列号
		if (sum > 0) 
		{					/* Add it up, move on */
			// 可读的数据累加
			amount += sum;
			// 如果是syn包则可读的减一，和上面的加一对称，因为syn包只是消耗了序列号，并不包含数据
			if (skb->h.th->syn) 
				amount--;
			// 消耗的序列号累加
			counted += sum;
		}
		/*
		 * Don't count urg data ... but do it in the right place!
		 * Consider: "old_data (ptr is here) URG PUSH data"
		 * The old code would stop at the first push because
		 * it counted the urg (amount==1) and then does amount--
		 * *after* the loop.  This means tcp_readable() always
		 * returned zero if any URG PUSH was in the queue, even
		 * though there was normal data available. If we subtract
		 * the urg data right here, we even get it to work for more
		 * than one URG PUSH skb without normal data.
		 * This means that select() finally works now with urg data
		 * in the queue.  Note that rlogin was never affected
		 * because it doesn't use select(); it uses two processes
		 * and a blocking read().  And the queue scan in tcp_read()
		 * was correct.  Mike <pall@rz.uni-karlsruhe.de>
		 */
		// 有紧急数据则减去紧急数据的一个字节
		if (skb->h.th->urg)
			amount--;	/* don't count urg data */
		if (amount && skb->h.th->psh) break;
		skb = skb->next;
	}
	while(skb != (struct sk_buff *)&sk->receive_queue);

	restore_flags(flags);
	if(sk->debug)
	  	printk("got %lu bytes.\n",amount);
	return(amount);
}

/*
 * LISTEN is a special case for select..
 */
// 对于listen型的socket，用于I/O复用函数select
static int tcp_listen_select(struct sock *sk, int sel_type, select_table *wait)
	{
	if (sel_type == SEL_IN) {
		int retval;

		sk->inuse = 1;
		retval = (tcp_find_established(sk) != NULL);
		release_sock(sk);
		// 没有建立的连接，阻塞等待唤醒
		if (!retval)
			select_wait(&master_select_wakeup,wait);
		return retval;
	}
	return 0;
}


/*
 *	Wait for a TCP event.
 *
 *	Note that we don't need to set "sk->inuse", as the upper select layers
 *	take care of normal races (between the test and the event) and we don't
 *	go look at any of the socket buffers directly.
 */
static int tcp_select(struct sock *sk, int sel_type, select_table *wait)
{	// 监听型的socket，则判断是否有可用的连接
	if (sk->state == TCP_LISTEN)
		return tcp_listen_select(sk, sel_type, wait);

	switch(sel_type) {
	// 是否有数据可读
	case SEL_IN:
		if (sk->err)
			return 1;
		// 还没建立起连接，没有数据可读
		if (sk->state == TCP_SYN_SENT || sk->state == TCP_SYN_RECV)
			break;

		if (sk->shutdown & RCV_SHUTDOWN)
			return 1;
		// 可读的等于已读的，没有数据可读			
		if (sk->acked_seq == sk->copied_seq)
			break;

		if (sk->urg_seq != sk->copied_seq ||
		    sk->acked_seq != sk->copied_seq+1 ||
		    sk->urginline || !sk->urg_data)
			return 1;
		break;
	// 能不能写
	case SEL_OUT:
		if (sk->err)
			return 1;
		// 已关闭不能写
		if (sk->shutdown & SEND_SHUTDOWN) 
			return 0;
		// 还没建立连接不能写
		if (sk->state == TCP_SYN_SENT || sk->state == TCP_SYN_RECV)
			break;
		/*
		 * This is now right thanks to a small fix
		 * by Matt Dillon.
		 */
		// 写空间不够不能写
		if (sk->prot->wspace(sk) < sk->mtu+128+sk->prot->max_header)
			break;
		return 1;

	case SEL_EX:
		if (sk->urg_data)
			return 1;
		break;
	}
	// 阻塞
	select_wait(sk->sleep, wait);
	return 0;
}

int tcp_ioctl(struct sock *sk, int cmd, unsigned long arg)
{
	int err;
	switch(cmd) 
	{

		case TIOCINQ:
#ifdef FIXME	/* FIXME: */
		case FIONREAD:
#endif
		{	
			// 可读的数据长度
			unsigned long amount;

			if (sk->state == TCP_LISTEN) 
				return(-EINVAL);

			sk->inuse = 1;
			amount = tcp_readable(sk);
			release_sock(sk);
			err=verify_area(VERIFY_WRITE,(void *)arg,
						   sizeof(unsigned long));
			if(err)
				return err;
			put_fs_long(amount,(unsigned long *)arg);
			return(0);
		}
		case SIOCATMARK:
		{	// 即将读取的是不是紧急数据
			int answ = sk->urg_data && sk->urg_seq == sk->copied_seq;

			err = verify_area(VERIFY_WRITE,(void *) arg,
						  sizeof(unsigned long));
			if (err)
				return err;
			put_fs_long(answ,(int *) arg);
			return(0);
		}
		case TIOCOUTQ:
		{
			unsigned long amount;
			// 还有多少写空间
			if (sk->state == TCP_LISTEN) return(-EINVAL);
			amount = sk->prot->wspace(sk);
			err=verify_area(VERIFY_WRITE,(void *)arg,
						   sizeof(unsigned long));
			if(err)
				return err;
			put_fs_long(amount,(unsigned long *)arg);
			return(0);
		}
		default:
			return(-EINVAL);
	}
}


/*
 *	This routine computes a TCP checksum. 
 */
// 检验和 
unsigned short tcp_check(struct tcphdr *th, int len,
	  unsigned long saddr, unsigned long daddr)
{     
	unsigned long sum;
   
	if (saddr == 0) saddr = ip_my_addr();

/*
 * stupid, gcc complains when I use just one __asm__ block,
 * something about too many reloads, but this is just two
 * instructions longer than what I want
 */
	__asm__("
	    addl %%ecx, %%ebx
	    adcl %%edx, %%ebx
	    adcl $0, %%ebx
	    "
	: "=b"(sum)
	: "0"(daddr), "c"(saddr), "d"((ntohs(len) << 16) + IPPROTO_TCP*256)
	: "bx", "cx", "dx" );
	__asm__("
	    movl %%ecx, %%edx
	    cld
	    cmpl $32, %%ecx
	    jb 2f
	    shrl $5, %%ecx
	    clc
1:	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    lodsl
	    adcl %%eax, %%ebx
	    loop 1b
	    adcl $0, %%ebx
	    movl %%edx, %%ecx
2:	    andl $28, %%ecx
	    je 4f
	    shrl $2, %%ecx
	    clc
3:	    lodsl
	    adcl %%eax, %%ebx
	    loop 3b
	    adcl $0, %%ebx
4:	    movl $0, %%eax
	    testw $2, %%dx
	    je 5f
	    lodsw
	    addl %%eax, %%ebx
	    adcl $0, %%ebx
	    movw $0, %%ax
5:	    test $1, %%edx
	    je 6f
	    lodsb
	    addl %%eax, %%ebx
	    adcl $0, %%ebx
6:	    movl %%ebx, %%eax
	    shrl $16, %%eax
	    addw %%ax, %%bx
	    adcw $0, %%bx
	    "
	: "=b"(sum)
	: "0"(sum), "c"(len), "S"(th)
	: "ax", "bx", "cx", "dx", "si" );

  	/* We only want the bottom 16 bits, but we never cleared the top 16. */
  
  	return((~sum) & 0xffff);
}


// 计算机校验和
void tcp_send_check(struct tcphdr *th, unsigned long saddr, 
		unsigned long daddr, int len, struct sock *sk)
{
	th->check = 0;
	th->check = tcp_check(th, len, saddr, daddr);
	return;
}

/*
 *	This is the main buffer sending routine. We queue the buffer
 *	having checked it is sane seeming.
 */
// 发送数据包 
static void tcp_send_skb(struct sock *sk, struct sk_buff *skb)
{
	int size;
	// 指向skb->data字段了的tcp头地址
	struct tcphdr * th = skb->h.th;

	/*
	 *	length of packet (not counting length of pre-tcp headers) 
	 */
	// tcp头+数据的长度
	size = skb->len - ((unsigned char *) th - skb->data);

	/*
	 *	Sanity check it.. 
	 */
	 
	if (size < sizeof(struct tcphdr) || size > skb->len) 
	{
		printk("tcp_send_skb: bad skb (skb = %p, data = %p, th = %p, len = %lu)\n",
			skb, skb->data, th, skb->len);
		kfree_skb(skb, FREE_WRITE);
		return;
	}

	/*
	 *	If we have queued a header size packet.. (these crash a few
	 *	tcp stacks if ack is not set)
	 */
	// 相等说明待发送的数据长度0
	if (size == sizeof(struct tcphdr)) 
	{
		/* If it's got a syn or fin it's notionally included in the size..*/
		// 不是syn或fin包则报错，只有这两种包的负载可以为0
		if(!th->syn && !th->fin) 
		{
			printk("tcp_send_skb: attempt to queue a bogon.\n");
			kfree_skb(skb,FREE_WRITE);
			return;
		}
	}

	/*
	 *	Actual processing.
	 */
	 
	tcp_statistics.TcpOutSegs++; 
	// size - 4*th->doff为数据负载的大小 
	skb->h.seq = ntohl(th->seq) + size - 4*th->doff;
	
	/*
	 *	We must queue if
	 *
	 *	a) The right edge of this frame exceeds the window
	 *	b) We are retransmitting (Nagle's rule)
	 *	c) We have too many packets 'in flight'
	 */
	// 包的序列号大于可以发送的最大序列号，正在进行超时重传（nagle算法规定只能有一个未收到确认的包，发出的包大于拥塞窗口了	 
	if (after(skb->h.seq, sk->window_seq) ||
	    (sk->retransmits && sk->ip_xmit_timeout == TIME_WRITE) ||
	     sk->packets_out >= sk->cong_window) 
	{
		/* checksum will be supplied by tcp_write_xmit.  So
		 * we shouldn't need to set it at all.  I'm being paranoid */
		th->check = 0;
		if (skb->next != NULL) 
		{
			printk("tcp_send_partial: next != NULL\n");
			skb_unlink(skb);
		}
		// 插入待发送队列
		skb_queue_tail(&sk->write_queue, skb);
		
		/*
		 *	If we don't fit we have to start the zero window
		 *	probes. This is broken - we really need to do a partial
		 *	send _first_ (This is what causes the Cisco and PC/TCP
		 *	grief).
		 */
		// 可发送的最大序列号小于包的序列号，并且没有等待确认的包，则需要发送窗口探测包看能不能继续发送数据 
		if (before(sk->window_seq, sk->write_queue.next->h.seq) &&
		    sk->send_head == NULL && sk->ack_backlog == 0)
			reset_xmit_timer(sk, TIME_PROBE0, sk->rto);
	} 
	else 
	{
		/*
		 *	This is going straight out
		 */
		// 希望对方传输的数据的序列化，即小于ack_seq的都收到了
		th->ack_seq = ntohl(sk->acked_seq);
		th->window = ntohs(tcp_select_window(sk));

		tcp_send_check(th, sk->saddr, sk->daddr, size, sk);
		// 将要发送的数据包第一个字节的序号 
		sk->sent_seq = sk->write_seq;
		
		/*
		 *	This is mad. The tcp retransmit queue is put together
		 *	by the ip layer. This causes half the problems with
		 *	unroutable FIN's and other things.
		 */
		// 使用ip_queue_xmit发送
		sk->prot->queue_xmit(sk, skb->dev, skb, 0);
		
		/*
		 *	Set for next retransmit based on expected ACK time.
		 *	FIXME: We set this every time which means our 
		 *	retransmits are really about a window behind.
		 */
		// 设置定时器用于超时重传
		reset_xmit_timer(sk, TIME_WRITE, sk->rto);
	}
}

/*
 *	Locking problems lead us to a messy situation where we can have
 *	multiple partially complete buffers queued up. This is really bad
 *	as we don't want to be sending partial buffers. Fix this with
 *	a semaphore or similar to lock tcp_write per socket.
 *
 *	These routines are pretty self descriptive.
 */
 
struct sk_buff * tcp_dequeue_partial(struct sock * sk)
{
	struct sk_buff * skb;
	unsigned long flags;

	save_flags(flags);
	cli();
	skb = sk->partial;
	// 只有一个包,取出来后直接置NULL，并且删除定时器，因为小包超时后也需要被强制发送出去
	if (skb) {
		sk->partial = NULL;
		del_timer(&sk->partial_timer);
	}
	restore_flags(flags);
	return skb;
}

/*
 *	Empty the partial queue
 */
 // 发送小的数据包
static void tcp_send_partial(struct sock *sk)
{
	struct sk_buff *skb;

	if (sk == NULL)
		return;
	while ((skb = tcp_dequeue_partial(sk)) != NULL)
		tcp_send_skb(sk, skb);
}

/*
 *	Queue a partial frame
 */
 
void tcp_enqueue_partial(struct sk_buff * skb, struct sock * sk)
{
	struct sk_buff * tmp;
	unsigned long flags;

	save_flags(flags);
	cli();
	// 保存待发送的skb
	tmp = sk->partial;
	// 准备发送，删除超时发送的定时器
	if (tmp)
		del_timer(&sk->partial_timer);
	// 保存新的skb
	sk->partial = skb;
	init_timer(&sk->partial_timer);
	/*
	 *	Wait up to 1 second for the buffer to fill.
	 */
	// 定时一秒后发送，一秒内如果有小块数据则继续填充到该skb
	sk->partial_timer.expires = HZ;
	sk->partial_timer.function = (void (*)(unsigned long)) tcp_send_partial;
	sk->partial_timer.data = (unsigned long) sk;
	add_timer(&sk->partial_timer);
	restore_flags(flags);
	// 发送旧的skb
	if (tmp)
		tcp_send_skb(sk, tmp);
}


/*
 *	This routine sends an ack and also updates the window. 
 */
 
static void tcp_send_ack(unsigned long sequence, unsigned long ack,
	     struct sock *sk,
	     struct tcphdr *th, unsigned long daddr)
{
	struct sk_buff *buff;
	struct tcphdr *t1;
	struct device *dev = NULL;
	int tmp;

	if(sk->zapped)
		return;		/* We have been reset, we may not send again */
		
	/*
	 * We need to grab some memory, and put together an ack,
	 * and then put it into the queue to be sent.
	 */
	// tcp+ip+mac头+4字节冗余校验
	buff = sk->prot->wmalloc(sk, MAX_ACK_SIZE, 1, GFP_ATOMIC);
	// 没有写空间了
	if (buff == NULL) 
	{
		/* 
		 *	Force it to send an ack. We don't have to do this
		 *	(ACK is unreliable) but it's much better use of 
		 *	bandwidth on slow links to send a spare ack than
		 *	resend packets. 
		 */
		// 累积的未确认个数
		sk->ack_backlog++;
		if (sk->ip_xmit_timeout != TIME_WRITE && tcp_connected(sk->state)) 
		{
			reset_xmit_timer(sk, TIME_WRITE, HZ);
		}
		return;
	}

	/*
	 *	Assemble a suitable TCP frame
	 */
	 
	buff->len = sizeof(struct tcphdr);
	buff->sk = sk;
	buff->localroute = sk->localroute;
	t1 =(struct tcphdr *) buff->data;

	/* 
	 *	Put in the IP header and routing stuff. 
	 */
	// 构造ip+mac头
	tmp = sk->prot->build_header(buff, sk->saddr, daddr, &dev,
				IPPROTO_TCP, sk->opt, MAX_ACK_SIZE,sk->ip_tos,sk->ip_ttl);
	if (tmp < 0) 
	{
  		buff->free = 1;
		sk->prot->wfree(sk, buff->mem_addr, buff->mem_len);
		return;
	}
	// 已用空间,tcp+ip+mac头大小
	buff->len += tmp;
	// 指向tcp头首地址
	t1 =(struct tcphdr *)((char *)t1 +tmp);
	// 复制tcp头
	memcpy(t1, th, sizeof(*t1));

	/*
	 *	Swap the send and the receive. 
	 */
	// 源地址、目的地址
	t1->dest = th->source;
	t1->source = th->dest;
	t1->seq = ntohl(sequence);
	t1->ack = 1;
	sk->window = tcp_select_window(sk);
	// 本机窗口大小
	t1->window = ntohs(sk->window);
	t1->res1 = 0;
	t1->res2 = 0;
	t1->rst = 0;
	t1->urg = 0;
	t1->syn = 0;
	t1->psh = 0;
	t1->fin = 0;
	
	/*
	 *	If we have nothing queued for transmit and the transmit timer
	 *	is on we are just doing an ACK timeout and need to switch
	 *	to a keepalive.
	 */
	// 确认的序列号等于当前累积的确认号
	if (ack == sk->acked_seq) 
	{	// 当前未累积的确认书置0
		sk->ack_backlog = 0;
		sk->bytes_rcv = 0;
		sk->ack_timed = 0;
		if (sk->send_head == NULL && skb_peek(&sk->write_queue) == NULL
				  && sk->ip_xmit_timeout == TIME_WRITE) 
		{
			if(sk->keepopen) {
				reset_xmit_timer(sk,TIME_KEEPOPEN,TCP_TIMEOUT_LEN);
			} else {
				delete_timer(sk);
			}
		}
  	}
  	
  	/*
  	 *	Fill in the packet and send it
  	 */
  	// 确认的序列号 
  	t1->ack_seq = ntohl(ack);
  	t1->doff = sizeof(*t1)/4;
	// 计算校验和
  	tcp_send_check(t1, sk->saddr, daddr, sizeof(*t1), sk);
  	if (sk->debug)
  		 printk("\rtcp_ack: seq %lx ack %lx\n", sequence, ack);
  	tcp_statistics.TcpOutSegs++;
	 // 发送
  	sk->prot->queue_xmit(sk, dev, buff, 1);
}


/* 
 *	This routine builds a generic TCP header. 
 */
// 构建tcp头
extern __inline int tcp_build_header(struct tcphdr *th, struct sock *sk, int push)
{

	memcpy(th,(void *) &(sk->dummy_th), sizeof(*th));
	// 序列号，即当前发送的数据中第一个字节的序号
	th->seq = htonl(sk->write_seq);
	// 设置协议栈是否马上把该数据包推到应用层，push为0说明当前的数据包是需要传输的数据中最后一个包
	th->psh =(push == 0) ? 1 : 0;
	// 头部长度
	th->doff = sizeof(*th)/4;
	th->ack = 1;
	th->fin = 0;
	sk->ack_backlog = 0;
	sk->bytes_rcv = 0;
	sk->ack_timed = 0;
	th->ack_seq = htonl(sk->acked_seq);
	sk->window = tcp_select_window(sk);
	th->window = htons(sk->window);

	return(sizeof(*th));
}

/*
 *	This routine copies from a user buffer into a socket,
 *	and starts the transmit system.
 */

static int tcp_write(struct sock *sk, unsigned char *from,
	  int len, int nonblock, unsigned flags)
{
	int copied = 0;
	int copy;
	int tmp;
	struct sk_buff *skb;
	struct sk_buff *send_tmp;
	unsigned char *buff;
	struct proto *prot;
	struct device *dev = NULL;

	sk->inuse=1;
	prot = sk->prot;
	while(len > 0) 
	{
		if (sk->err) 
		{			/* Stop on an error */
			release_sock(sk);
			if (copied) 
				return(copied);
			tmp = -sk->err;
			sk->err = 0;
			return(tmp);
		}

		/*
		 *	First thing we do is make sure that we are established. 
		 */
		// 关闭了只能读不能写
		if (sk->shutdown & SEND_SHUTDOWN) 
		{
			release_sock(sk);
			sk->err = EPIPE;
			if (copied) 
				return(copied);
			sk->err = 0;
			return(-EPIPE);
		}

		/* 
		 *	Wait for a connection to finish.
		 */
		// 处于不能写状态，close_wait是可写不可读，因为对端已经关闭了写		
		while(sk->state != TCP_ESTABLISHED && sk->state != TCP_CLOSE_WAIT) 
		{
			if (sk->err) 
			{
				release_sock(sk);
				if (copied) 
					return(copied);
				tmp = -sk->err;
				sk->err = 0;
				return(tmp);
			}
			// syn和syn_recv状态的时候可以写，重复发包，否则是出错状态
			if (sk->state != TCP_SYN_SENT && sk->state != TCP_SYN_RECV) 
			{
				release_sock(sk);
				if (copied) 
					return(copied);

				if (sk->err) 
				{
					tmp = -sk->err;
					sk->err = 0;
					return(tmp);
				}
				// 长连接 
				if (sk->keepopen) 
				{
					send_sig(SIGPIPE, current, 0);
				}
				return(-EPIPE);
			}

			if (nonblock || copied) 
			{
				release_sock(sk);
				if (copied) 
					return(copied);
				return(-EAGAIN);
			}

			release_sock(sk);
			cli();
		
			if (sk->state != TCP_ESTABLISHED &&
		    		sk->state != TCP_CLOSE_WAIT && sk->err == 0) 
		    	{
				interruptible_sleep_on(sk->sleep);
				if (current->signal & ~current->blocked) 
				{
					sti();
					if (copied) 
						return(copied);
					return(-ERESTARTSYS);
				}
			}
			sk->inuse = 1;
			sti();
		}

	/*
	 * The following code can result in copy <= if sk->mss is ever
	 * decreased.  It shouldn't be.  sk->mss is min(sk->mtu, sk->max_window).
	 * sk->mtu is constant once SYN processing is finished.  I.e. we
	 * had better not get here until we've seen his SYN and at least one
	 * valid ack.  (The SYN sets sk->mtu and the ack sets sk->max_window.)
	 * But ESTABLISHED should guarantee that.  sk->max_window is by definition
	 * non-decreasing.  Note that any ioctl to set user_mss must be done
	 * before the exchange of SYN's.  If the initial ack from the other
	 * end has a window of 0, max_window and thus mss will both be 0.
	 */

	/* 
	 *	Now we need to check if we have a half built packet. 
	 */
		// 先看是否有小块的数据被缓存起来，是的话先取出skb，不需要立刻发送的话再入队
		if ((skb = tcp_dequeue_partial(sk)) != NULL) 
		{
		        int hdrlen;

		         /* IP header + TCP header */
			// 所有协议头的长度
			hdrlen = ((unsigned long)skb->h.th - (unsigned long)skb->data)
			         + sizeof(struct tcphdr);
	
			/* Add more stuff to the end of skb->len */
			// 不是紧急数据，则把数据追加到缓存的小包数据后面，是紧急数据则先把小包数据发出去，然后下一个循环再发普通数据
			if (!(flags & MSG_OOB)) 
			{	
				// mss-数据长度等于还可以传多少长度的数据
				copy = min(sk->mss - (skb->len - hdrlen), len);
				/* FIXME: this is really a bug. */
				if (copy <= 0) 
				{
			  		printk("TCP: **bug**: \"copy\" <= 0!!\n");
			  		copy = 0;
				}
	  			// 把用户的数据赋值copy长度个字节到数据包的数据部分
				memcpy_fromfs(skb->data + skb->len, from, copy);
				// 更新skb的data字段使用了多少字节
				skb->len += copy;
				// 下次复制的首地址
				from += copy;
				// 已复制的字节长度
				copied += copy;
				// 还有多少字节需要复制
				len -= copy;
				// 下一个发送的字节的序列号大小
				sk->write_seq += copy;
			}
			// 数据部分大于等于mss或者是带外数据或者还没有发出去一个数据包则直接发送
			if ((skb->len - hdrlen) >= sk->mss ||
				(flags & MSG_OOB) || !sk->packets_out)
				tcp_send_skb(sk, skb);
			else
				// 继续缓存，满足条件后一起发送
				tcp_enqueue_partial(skb, sk);
			continue;
		}

	/*
	 * We also need to worry about the window.
 	 * If window < 1/2 the maximum window we've seen from this
 	 *   host, don't use it.  This is sender side
 	 *   silly window prevention, as specified in RFC1122.
 	 *   (Note that this is different than earlier versions of
 	 *   SWS prevention, e.g. RFC813.).  What we actually do is 
	 *   use the whole MSS.  Since the results in the right
	 *   edge of the packet being outside the window, it will
	 *   be queued for later rather than sent.
	 */
		// 可发送的序列化最大值 - 下一个可写的序列化值等于可以发送的字节数
		copy = sk->window_seq - sk->write_seq;
		if (copy <= 0 || copy < (sk->max_window >> 1) || copy > sk->mss)
			copy = sk->mss;
		// 能发送的比需要发送的大，则取需要发送的
		if (copy > len)
			copy = len;

	/*
	 *	We should really check the window here also. 
	 */
	 
		send_tmp = NULL;
		// 不是紧急数据并且也小于mss，则需要缓存到partial队列，否则直接发送
		if (copy < sk->mss && !(flags & MSG_OOB)) 
		{
			/*
			 *	We will release the socket in case we sleep here. 
			 */
			release_sock(sk);
			/*
			 *	NB: following must be mtu, because mss can be increased.
			 *	mss is always <= mtu 
			 */
			skb = prot->wmalloc(sk, sk->mtu + 128 + prot->max_header, 0, GFP_KERNEL);
			sk->inuse = 1;
			send_tmp = skb;
		} 
		else 
		{
			/*
			 *	We will release the socket in case we sleep here. 
			 */
			release_sock(sk);
			skb = prot->wmalloc(sk, copy + prot->max_header , 0, GFP_KERNEL);
  			sk->inuse = 1;
		}

		/*
		 *	If we didn't get any memory, we need to sleep. 
		 */
		// 没有写空间了
		if (skb == NULL) 
		{
			sk->socket->flags |= SO_NOSPACE;
			// 非阻塞直接返回已经写入的字节
			if (nonblock) 
			{
				release_sock(sk);
				if (copied) 
					return(copied);
				return(-EAGAIN);
			}

			/*
			 *	FIXME: here is another race condition. 
			 */

			tmp = sk->wmem_alloc;
			// 这个函数会处理收到的数据包，如果收到ack包则会腾出写空间
			release_sock(sk);
			cli();
			/*
			 *	Again we will try to avoid it. 
			 */
			// 处于可写状态但是没有写空间，则阻塞
			if (tmp <= sk->wmem_alloc &&
				  (sk->state == TCP_ESTABLISHED||sk->state == TCP_CLOSE_WAIT)
				&& sk->err == 0) 
			{
				sk->socket->flags &= ~SO_NOSPACE;
				interruptible_sleep_on(sk->sleep);
				if (current->signal & ~current->blocked) 
				{
					sti();
					if (copied) 
						return(copied);
					return(-ERESTARTSYS);
				}
			}
			sk->inuse = 1;
			sti();
			continue;
		}

		skb->len = 0;
		skb->sk = sk;
		skb->free = 0;
		skb->localroute = sk->localroute|(flags&MSG_DONTROUTE);
	
		buff = skb->data;
	
		/*
		 * FIXME: we need to optimize this.
		 * Perhaps some hints here would be good.
		 */
		// 构建ip头和mac头，返回ip头+mac头的长度的大小,查找路由项的时候会给dev赋值
		tmp = prot->build_header(skb, sk->saddr, sk->daddr, &dev,
				 IPPROTO_TCP, sk->opt, skb->mem_len,sk->ip_tos,sk->ip_ttl);
		if (tmp < 0 ) 
		{
			prot->wfree(sk, skb->mem_addr, skb->mem_len);
			release_sock(sk);
			if (copied) 
				return(copied);
			return(tmp);
		}
		// 更新data中的数据长度
		skb->len += tmp;
		skb->dev = dev;
		// 指向可写地址，准备写入tcp头
		buff += tmp;
		// skb的tcp头指向data字段的tcp头
		skb->h.th =(struct tcphdr *) buff;
		// 构建tcp头，len-copy表示是否已经传输完len字节的数据，用于设置push标记
		tmp = tcp_build_header((struct tcphdr *)buff, sk, len-copy);
		if (tmp < 0) 
		{
			prot->wfree(sk, skb->mem_addr, skb->mem_len);
			release_sock(sk);
			if (copied) 
				return(copied);
			return(tmp);
		}
		// 带外数据
		if (flags & MSG_OOB) 
		{	// 设置urg标记位，设置紧急指针指向紧急数据的后面一个字节
			((struct tcphdr *)buff)->urg = 1;
			((struct tcphdr *)buff)->urg_ptr = ntohs(copy);
		}
		// 更新skb->data中的数据长度
		skb->len += tmp;
		// 复制copy个字节到tcp头后面成为tcp报文的负载
		memcpy_fromfs(buff+tmp, from, copy);
		// 更新需要复制的数据地址
		from += copy;
		// 复制字节数累加
		copied += copy;
		// 还有多少个字节需要复制
		len -= copy;
		// 更新skb->data的数据长度
		skb->len += copy;
		skb->free = 0;
		// 更新下一个tcp报文的序列化
		sk->write_seq += copy;
		// 数据量太少并且不是紧急数据，并且有待确认的包（nagle算法规则），则先缓存
		if (send_tmp != NULL && sk->packets_out) 
		{
			tcp_enqueue_partial(send_tmp, sk);
			continue;
		}
		// 否则直接发送
		tcp_send_skb(sk, skb);
	}
	sk->err = 0;

/*
 *	Nagle's rule. Turn Nagle off with TCP_NODELAY for highly
 *	interactive fast network servers. It's meant to be on and
 *	it really improves the throughput though not the echo time
 *	on my slow slip link - Alan
 */

/*
 *	Avoid possible race on send_tmp - c/o Johannes Stille 
 */
	// 符合nagle算法条件或者没有开启nagle算法且序列号合法则发送
	if(sk->partial && ((!sk->packets_out) 
     /* If not nagling we can send on the before case too.. */
	      || (sk->nonagle && before(sk->write_seq , sk->window_seq))
      	))
  		tcp_send_partial(sk);

	release_sock(sk);
	return(copied);
}

/*
 *	This is just a wrapper. 
 */

static int tcp_sendto(struct sock *sk, unsigned char *from,
	   int len, int nonblock, unsigned flags,
	   struct sockaddr_in *addr, int addr_len)
{	
	// 只支持两个flag
	if (flags & ~(MSG_OOB|MSG_DONTROUTE))
		return -EINVAL;
	if (sk->state == TCP_CLOSE)
		return -ENOTCONN;
	if (addr_len < sizeof(*addr))
		return -EINVAL;
	if (addr->sin_family && addr->sin_family != AF_INET) 
		return -EINVAL;
	if (addr->sin_port != sk->dummy_th.dest) 
		return -EISCONN;
	if (addr->sin_addr.s_addr != sk->daddr) 
		return -EISCONN;
	return tcp_write(sk, from, len, nonblock, flags);
}


/*
 *	Send an ack if one is backlogged at this point. Ought to merge
 *	this with tcp_send_ack().
 */
 
static void tcp_read_wakeup(struct sock *sk)
{
	int tmp;
	struct device *dev = NULL;
	struct tcphdr *t1;
	struct sk_buff *buff;
	// 没有待确认的数据包
	if (!sk->ack_backlog) 
		return;

	/*
	 * If we're closed, don't send an ack, or we'll get a RST
	 * from the closed destination.
	 */
	// 连接关闭就不需要发了
	if ((sk->state == TCP_CLOSE) || (sk->state == TCP_TIME_WAIT))
		return; 

	/*
	 * FIXME: we need to put code here to prevent this routine from
	 * being called.  Being called once in a while is ok, so only check
	 * if this is the second time in a row.
 	 */

	/*
	 * We need to grab some memory, and put together an ack,
	 * and then put it into the queue to be sent.
	 */

	buff = sk->prot->wmalloc(sk,MAX_ACK_SIZE,1, GFP_ATOMIC);
	if (buff == NULL) 
	{
		/* Try again real soon. */
		reset_xmit_timer(sk, TIME_WRITE, HZ);
		return;
 	}

	buff->len = sizeof(struct tcphdr);
	buff->sk = sk;
	buff->localroute = sk->localroute;
	
	/*
	 *	Put in the IP header and routing stuff. 
	 */

	tmp = sk->prot->build_header(buff, sk->saddr, sk->daddr, &dev,
			       IPPROTO_TCP, sk->opt, MAX_ACK_SIZE,sk->ip_tos,sk->ip_ttl);
	if (tmp < 0) 
	{
  		buff->free = 1;
		sk->prot->wfree(sk, buff->mem_addr, buff->mem_len);
		return;
	}

	buff->len += tmp;
	t1 =(struct tcphdr *)(buff->data +tmp);

	memcpy(t1,(void *) &sk->dummy_th, sizeof(*t1));
	t1->seq = htonl(sk->sent_seq);
	t1->ack = 1;
	t1->res1 = 0;
	t1->res2 = 0;
	t1->rst = 0;
	t1->urg = 0;
	t1->syn = 0;
	t1->psh = 0;
	// 重置待确认数据包个数
	sk->ack_backlog = 0;
	sk->bytes_rcv = 0;
	sk->window = tcp_select_window(sk);
	t1->window = ntohs(sk->window);
	t1->ack_seq = ntohl(sk->acked_seq);
	t1->doff = sizeof(*t1)/4;
	tcp_send_check(t1, sk->saddr, sk->daddr, sizeof(*t1), sk);
	sk->prot->queue_xmit(sk, dev, buff, 1);
	tcp_statistics.TcpOutSegs++;
}


/*
 * 	FIXME:
 * 	This routine frees used buffers.
 * 	It should consider sending an ACK to let the
 * 	other end know we now have a bigger window.
 */

static void cleanup_rbuf(struct sock *sk)
{
	unsigned long flags;
	unsigned long left;
	struct sk_buff *skb;
	unsigned long rspace;

	if(sk->debug)
	  	printk("cleaning rbuf for sk=%p\n", sk);
  
	save_flags(flags);
	cli();
	// 还有多少读空间
	left = sk->prot->rspace(sk);
 
	/*
	 *	We have to loop through all the buffer headers,
	 *	and try to free up all the space we can.
	 */

	while((skb=skb_peek(&sk->receive_queue)) != NULL) 
	{
		if (!skb->used || skb->users) 
			break;
		skb_unlink(skb);
		skb->sk = sk;
		kfree_skb(skb, FREE_READ);
	}

	restore_flags(flags);

	/*
	 *	FIXME:
	 *	At this point we should send an ack if the difference
	 *	in the window, and the amount of space is bigger than
	 *	TCP_WINDOW_DIFF.
	 */

	if(sk->debug)
		printk("sk->rspace = %lu, was %lu\n", sk->prot->rspace(sk),
  					    left);
	if ((rspace=sk->prot->rspace(sk)) != left) 
	{
		/*
		 * This area has caused the most trouble.  The current strategy
		 * is to simply do nothing if the other end has room to send at
		 * least 3 full packets, because the ack from those will auto-
		 * matically update the window.  If the other end doesn't think
		 * we have much space left, but we have room for at least 1 more
		 * complete packet than it thinks we do, we will send an ack
		 * immediately.  Otherwise we will wait up to .5 seconds in case
		 * the user reads some more.
		 */
		sk->ack_backlog++;
	/*
	 * It's unclear whether to use sk->mtu or sk->mss here.  They differ only
	 * if the other end is offering a window smaller than the agreed on MSS
	 * (called sk->mtu here).  In theory there's no connection between send
	 * and receive, and so no reason to think that they're going to send
	 * small packets.  For the moment I'm using the hack of reducing the mss
	 * only on the send side, so I'm putting mtu here.
	 */

		if (rspace > (sk->window - sk->bytes_rcv + sk->mtu)) 
		{
			/* Send an ack right now. */
			tcp_read_wakeup(sk);
		} 
		else 
		{
			/* Force it to send an ack soon. */
			int was_active = del_timer(&sk->retransmit_timer);
			if (!was_active || TCP_ACK_TIME < sk->timer.expires) 
			{
				reset_xmit_timer(sk, TIME_WRITE, TCP_ACK_TIME);
			} 
			else
				add_timer(&sk->retransmit_timer);
		}
	}
} 


/*
 *	Handle reading urgent data. BSD has very simple semantics for
 *	this, no blocking and very strange errors 8)
 */
 
static int tcp_read_urg(struct sock * sk, int nonblock,
	     unsigned char *to, int len, unsigned flags)
{
	/*
	 *	No URG data to read
	 */
	// 没有紧急数据或者紧急数据被读取了又或者紧急数据当作普通数据处理了
	if (sk->urginline || !sk->urg_data || sk->urg_data == URG_READ)
		return -EINVAL;	/* Yes this is right ! */
		
	if (sk->err) 
	{
		int tmp = -sk->err;
		sk->err = 0;
		return tmp;
	}

	if (sk->state == TCP_CLOSE || sk->done) 
	{
		if (!sk->done) {
			sk->done = 1;
			return 0;
		}
		return -ENOTCONN;
	}

	if (sk->shutdown & RCV_SHUTDOWN) 
	{
		sk->done = 1;
		return 0;
	}
	sk->inuse = 1;
	// urg_data是两个字节，一个字节保存紧急数据有效标记，一个保存一个字节的紧急数据
	if (sk->urg_data & URG_VALID) 
	{
		char c = sk->urg_data;
		// 如果不是预读，则读完后设置已读，下次读的时候就直接返回错误
		if (!(flags & MSG_PEEK))
			sk->urg_data = URG_READ;
		// 只读一个字节
		put_fs_byte(c, to);
		release_sock(sk);
		return 1;
	}
	release_sock(sk);
	
	/*
	 * Fixed the recv(..., MSG_OOB) behaviour.  BSD docs and
	 * the available implementations agree in this case:
	 * this call should never block, independent of the
	 * blocking state of the socket.
	 * Mike <pall@rz.uni-karlsruhe.de>
	 */
	return -EAGAIN;
}


/*
 *	This routine copies from a sock struct into the user buffer. 
 */
 
static int tcp_read(struct sock *sk, unsigned char *to,
	int len, int nonblock, unsigned flags)
{
	struct wait_queue wait = { current, NULL };
	int copied = 0;
	unsigned long peek_seq;
	volatile unsigned long *seq;	/* So gcc doesn't overoptimise */
	unsigned long used;

	/* 
	 *	This error should be checked. 
	 */
	 
	if (sk->state == TCP_LISTEN)
		return -ENOTCONN;

	/*
	 *	Urgent data needs to be handled specially. 
	 */
	 
	if (flags & MSG_OOB)
		return tcp_read_urg(sk, nonblock, to, len, flags);

	/*
	 *	Copying sequence to update. This is volatile to handle
	 *	the multi-reader case neatly (memcpy_to/fromfs might be 
	 *	inline and thus not flush cached variables otherwise).
	 */
	// 应用层可以读但还没读的数据的第一个序列号 
	peek_seq = sk->copied_seq;
	seq = &sk->copied_seq;
	// 读取但是不从缓冲区中移除，前瞻
	if (flags & MSG_PEEK)
		seq = &peek_seq;

	add_wait_queue(sk->sleep, &wait);
	sk->inuse = 1;
	while (len > 0) 
	{
		struct sk_buff * skb;
		unsigned long offset;
	
		/*
		 * Are we at urgent data? Stop if we have read anything.
		 */
		 
		if (copied && sk->urg_data && sk->urg_seq == *seq)
			break;

		/*
		 *	Next get a buffer.
		 */
		 
		current->state = TASK_INTERRUPTIBLE;

		skb = skb_peek(&sk->receive_queue);
		do 
		{
			if (!skb)
				break;
			// 小于当前skb的序列号说明要读的数据已经读过了，大于seq说明当前skb可能是需要读取的数据，取决于skb->len
			if (before(*seq, skb->h.th->seq))
				break;
			// offset表示当前数据包中已经被读取了的字节数，如果skb->len大于offset说明该skb的数据是需要被读取的
			offset = *seq - skb->h.th->seq;
			if (skb->h.th->syn)
				offset--;
			if (offset < skb->len)
				goto found_ok_skb;
			if (skb->h.th->fin)
				goto found_fin_ok;
			if (!(flags & MSG_PEEK))
				skb->used = 1;
			skb = skb->next;
		}
		while (skb != (struct sk_buff *)&sk->receive_queue);

		if (copied)
			break;

		if (sk->err) 
		{
			copied = -sk->err;
			sk->err = 0;
			break;
		}

		if (sk->state == TCP_CLOSE) 
		{
			if (!sk->done) 
			{
				sk->done = 1;
				break;
			}
			copied = -ENOTCONN;
			break;
		}

		if (sk->shutdown & RCV_SHUTDOWN) 
		{
			sk->done = 1;
			break;
		}
			
		if (nonblock) 
		{
			copied = -EAGAIN;
			break;
		}

		cleanup_rbuf(sk);
		release_sock(sk);
		sk->socket->flags |= SO_WAITDATA;
		schedule();
		sk->socket->flags &= ~SO_WAITDATA;
		sk->inuse = 1;

		if (current->signal & ~current->blocked) 
		{
			copied = -ERESTARTSYS;
			break;
		}
		continue;

	found_ok_skb:
		/*
		 *	Lock the buffer. We can be fairly relaxed as
		 *	an interrupt will never steal a buffer we are 
		 *	using unless I've missed something serious in
		 *	tcp_data.
		 */
		
		skb->users++;
		
		/*
		 *	Ok so how much can we use ? 
		 */
		//  数据总数-已经被读取的=可以被读取的大小
		used = skb->len - offset;
		// 要读的比可以读的少，取小的
		if (len < used)
			used = len;
		/*
		 *	Do we have urgent data here? 
		 */
		
		if (sk->urg_data) 
		{	
			// 紧急数据离已经读取的数据之间还有多少个字节
			unsigned long urg_offset = sk->urg_seq - *seq;
			// 小于可以读取的数据说明紧急数据在该数据包中
			if (urg_offset < used) 
			{	
				// 下一个要读的字节就是紧急数据，即urg_offset等于0
				if (!urg_offset) 
				{	// 紧急数据没有被当做普通数据处理则跳过紧急数据
					if (!sk->urginline) 
					{	
						// 要读取的序列号加一
						++*seq;
						// 位置加一
						offset++;
						// 可读的字节数减一
						used--;
					}
				}
				else
					// 指向紧急数据那个字节的的位置，先读到紧急数据前一个字节，下次读的时候跳过紧急数据再读下紧急数据的一个字节
					used = urg_offset;
			}
		}
		
		/*
		 *	Copy it - We _MUST_ update *seq first so that we
		 *	don't ever double read when we have dual readers
		 */
		// 更新下一个要读的序列号		 
		*seq += used;

		/*
		 *	This memcpy_tofs can sleep. If it sleeps and we
		 *	do a second read it relies on the skb->users to avoid
		 *	a crash when cleanup_rbuf() gets called.
		 */
		// tcp头加上offset即加上当前数据包中已经被读取的字节个数，即指向第一个需要被读的字节，used是可读字节数
		memcpy_tofs(to,((unsigned char *)skb->h.th) +
			skb->h.th->doff*4 + offset, used);
		// 读取字节数累加
		copied += used;
		// 更新还需要读取的字节数
		len -= used;
		// 更新下一个写入的位置
		to += used;
		
		/*
		 *	We now will not sleep again until we are finished
		 *	with skb. Sorry if you are doing the SMP port
		 *	but you'll just have to fix it neatly ;)
		 */
		 
		skb->users --;
		// 下一个要读取的序列号在urg_seq的后面说明紧急数据已经被读取，清空
		if (after(sk->copied_seq,sk->urg_seq))
			sk->urg_data = 0;
		// 当前数据包的数据还没被读完，继续读
		if (used + offset < skb->len)
			continue;
		
		/*
		 *	Process the FIN.
		 */

		if (skb->h.th->fin)
			goto found_fin_ok;
		if (flags & MSG_PEEK)
			continue;
		skb->used = 1;
		continue;

	found_fin_ok:
		++*seq;
		if (flags & MSG_PEEK)
			break;
			
		/*
		 *	All is done
		 */
		 
		skb->used = 1;
		sk->shutdown |= RCV_SHUTDOWN;
		break;

	}
	remove_wait_queue(sk->sleep, &wait);
	current->state = TASK_RUNNING;

	/* Clean up data we have read: This will do ACK frames */
	cleanup_rbuf(sk);
	release_sock(sk);
	return copied;
}

/*
 *	State processing on a close. This implements the state shift for
 *	sending our FIN frame. Note that we only send a FIN for some 
 *	states. A shutdown() may have already sent the FIN, or we may be
 *	closed.
 */
// 根据socket的当前状态修改下一个状态 
static int tcp_close_state(struct sock *sk, int dead)
{	
	// 默认状态是关闭
	int ns=TCP_CLOSE;
	// 默认不需要发送fin包
	int send_fin=0;
	switch(sk->state)
	{   // 还没有建立连接，直接转为关闭状态
		case TCP_SYN_SENT:	/* No SYN back, no FIN needed */
			break;
		// 收到syn包并且发出了ack，对方也可能已经收到了ack把状态置为已建立，所以需要发送fin包，并把状态置为fin_wait1
		case TCP_SYN_RECV:
		case TCP_ESTABLISHED:	/* Closedown begin */
			ns=TCP_FIN_WAIT1;
			send_fin=1;
			break;
		// 本端已关闭，即已经发送了fin包，不需要再发送了，状态不变
		case TCP_FIN_WAIT1:	/* Already closing, or FIN sent: no change */
		case TCP_FIN_WAIT2:
		case TCP_CLOSING:
			ns=sk->state;
			break;
		// 直接置为关闭状态
		case TCP_CLOSE:
		case TCP_LISTEN:
			break;
		// 对端已关闭，现在是本端准备关闭，需要发送fin包，然后进入last_ack状态
		case TCP_CLOSE_WAIT:	/* They have FIN'd us. We send our FIN and
					   wait only for the ACK */
			ns=TCP_LAST_ACK;
			send_fin=1;
	}
	
	tcp_set_state(sk,ns);
		
	/*
	 *	This is a (useful) BSD violating of the RFC. There is a
	 *	problem with TCP as specified in that the other end could
	 *	keep a socket open forever with no application left this end.
	 *	We use a 3 minute timeout (about the same as BSD) then kill
	 *	our end. If they send after that then tough - BUT: long enough
	 *	that we won't make the old 4*rto = almost no time - whoops
	 *	reset mistake.
	 */
	// 如果是本端已经关闭，在等待对端关闭的状态，则设置一个定时器，如果超时还没有收到对端的fin包则强行关闭
	if(dead && ns==TCP_FIN_WAIT2)
	{
		int timer_active=del_timer(&sk->timer);
		if(timer_active)
			add_timer(&sk->timer);
		else
			reset_msl_timer(sk, TIME_CLOSE, TCP_FIN_TIMEOUT);
	}
	
	return send_fin;
}

/*
 *	Send a fin.
 */
// 发送fin包
static void tcp_send_fin(struct sock *sk)
{
	struct proto *prot =(struct proto *)sk->prot;
	struct tcphdr *th =(struct tcphdr *)&sk->dummy_th;
	struct tcphdr *t1;
	struct sk_buff *buff;
	struct device *dev=NULL;
	int tmp;
		
	release_sock(sk); /* in case the malloc sleeps. */
	// 分配一个用于写的skb	
	buff = prot->wmalloc(sk, MAX_RESET_SIZE,1 , GFP_KERNEL);
	sk->inuse = 1;

	if (buff == NULL)
	{
		/* This is a disaster if it occurs */
		printk("tcp_send_fin: Impossible malloc failure");
		return;
	}

	/*
	 *	Administrivia
	 */
	 
	buff->sk = sk;
	// 当前已用的大小，一个tcp头，内容在下面赋值
	buff->len = sizeof(*t1);
	buff->localroute = sk->localroute;
	// 指向可写的地址
	t1 =(struct tcphdr *) buff->data;

	/*
	 *	Put in the IP header and routing stuff. 
	 */
	// 构建IP头、MAC头
	tmp = prot->build_header(buff,sk->saddr, sk->daddr, &dev,
			   IPPROTO_TCP, sk->opt,
			   sizeof(struct tcphdr),sk->ip_tos,sk->ip_ttl);
	if (tmp < 0) 
	{
		int t;
  		/*
  		 *	Finish anyway, treat this as a send that got lost. 
  		 *	(Not good).
  		 */
  		 
	  	buff->free = 1;
		prot->wfree(sk,buff->mem_addr, buff->mem_len);
		sk->write_seq++;
		t=del_timer(&sk->timer);
		if(t)
			add_timer(&sk->timer);
		else
			reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
		return;
	}
	
	/*
	 *	We ought to check if the end of the queue is a buffer and
	 *	if so simply add the fin to that buffer, not send it ahead.
	 */
	// 指向下一个可以写的地址
	t1 =(struct tcphdr *)((char *)t1 +tmp);
	// 更新已使用的大小
	buff->len += tmp;
	buff->dev = dev;
	// 写入tcp头的内容
	memcpy(t1, th, sizeof(*t1));
	// 序列号
	t1->seq = ntohl(sk->write_seq);
	// 更新序列号
	sk->write_seq++;
	buff->h.seq = sk->write_seq;
	// 也是一个ack包
	t1->ack = 1;
	// 期待收到对端的下一个字节的序列号
	t1->ack_seq = ntohl(sk->acked_seq);
	// 当前接收窗口的大小
	t1->window = ntohs(sk->window=tcp_select_window(sk));
	// 是个fin包
	t1->fin = 1;
	t1->rst = 0;
	// tcp头长度
	t1->doff = sizeof(*t1)/4;
	tcp_send_check(t1, sk->saddr, sk->daddr, sizeof(*t1), sk);

	/*
	 * If there is data in the write queue, the fin must be appended to
	 * the write queue.
 	 */
	// 还有数据没有发出去
 	if (skb_peek(&sk->write_queue) != NULL) 
 	{
  		buff->free = 0;
		if (buff->next != NULL) 
		{
			printk("tcp_send_fin: next != NULL\n");
			skb_unlink(buff);
		}
		// 放到写队列末尾，等到前面的数据先发出去
		skb_queue_tail(&sk->write_queue, buff);
  	} 
  	else 
  	{		// 立刻发出去，更新下一个数据包发送时第一个字节的序列号
        	sk->sent_seq = sk->write_seq;
		sk->prot->queue_xmit(sk, dev, buff, 0);
		// 重置定时器为rto
		reset_xmit_timer(sk, TIME_WRITE, sk->rto);
	}
}

/*
 *	Shutdown the sending side of a connection. Much like close except
 *	that we don't receive shut down or set sk->dead=1.
 */
// 半关闭
void tcp_shutdown(struct sock *sk, int how)
{
	/*
	 *	We need to grab some memory, and put together a FIN,
	 *	and then put it into the queue to be sent.
	 *		Tim MacKenzie(tym@dibbler.cs.monash.edu.au) 4 Dec '92.
	 */

	if (!(how & SEND_SHUTDOWN)) 
		return;
	 
	/*
	 *	If we've already sent a FIN, or it's a closed state
	 */
	// 已经关闭了写端或者是监听套接字则返回，不需要关闭了
	if (sk->state == TCP_FIN_WAIT1 ||
	    sk->state == TCP_FIN_WAIT2 ||
	    sk->state == TCP_CLOSING ||
	    sk->state == TCP_LAST_ACK ||
	    sk->state == TCP_TIME_WAIT || 
	    sk->state == TCP_CLOSE ||
	    sk->state == TCP_LISTEN
	  )
	{
		return;
	}
	sk->inuse = 1;

	/*
	 * flag that the sender has shutdown
	 */
	// 设置flag
	sk->shutdown |= SEND_SHUTDOWN;

	/*
	 *  Clear out any half completed packets. 
	 */
	// 还有小数据包则发送出去
	if (sk->partial)
		tcp_send_partial(sk);
		
	/*
	 *	FIN if needed
	 */
	// 关闭socket，根据socket的状态判断是不是需要发送fin包 
	if(tcp_close_state(sk,0))
		tcp_send_fin(sk);
		
	release_sock(sk);
}

// 读取数据并且获取对端地址
static int
tcp_recvfrom(struct sock *sk, unsigned char *to,
	     int to_len, int nonblock, unsigned flags,
	     struct sockaddr_in *addr, int *addr_len)
{
	int result;
  
	/* 
	 *	Have to check these first unlike the old code. If 
	 *	we check them after we lose data on an error
	 *	which is wrong 
	 */

	if(addr_len)
		*addr_len = sizeof(*addr);
	result=tcp_read(sk, to, to_len, nonblock, flags);

	if (result < 0) 
		return(result);
  
  	if(addr)
  	{
		addr->sin_family = AF_INET;
 		addr->sin_port = sk->dummy_th.dest;
		addr->sin_addr.s_addr = sk->daddr;
	}
	return(result);
}


/*
 *	This routine will send an RST to the other tcp. 
 */
 // 发送重置包
static void tcp_reset(unsigned long saddr, unsigned long daddr, struct tcphdr *th,
	  struct proto *prot, struct options *opt, struct device *dev, int tos, int ttl)
{
	struct sk_buff *buff;
	struct tcphdr *t1;
	int tmp;
	struct device *ndev=NULL;

	/*
	 *	Cannot reset a reset (Think about it).
	 */
	// 收到rst包不能发送rst回去，否则会死循环，双方会发来发去
	if(th->rst)
		return;
  
	/*
	 * We need to grab some memory, and put together an RST,
	 * and then put it into the queue to be sent.
	 */
	// 分配一个skb
	buff = prot->wmalloc(NULL, MAX_RESET_SIZE, 1, GFP_ATOMIC);
	if (buff == NULL) 
	  	return;

	buff->len = sizeof(*t1);
	buff->sk = NULL;
	buff->dev = dev;
	buff->localroute = 0;

	t1 =(struct tcphdr *) buff->data;

	/*
	 *	Put in the IP header and routing stuff. 
	 */

	tmp = prot->build_header(buff, saddr, daddr, &ndev, IPPROTO_TCP, opt,
			   sizeof(struct tcphdr),tos,ttl);
	// 构建ip和mac头失败则释放刚才申请的内存	
	if (tmp < 0) 
	{
  		buff->free = 1;
		prot->wfree(NULL, buff->mem_addr, buff->mem_len);
		return;
	}

	t1 =(struct tcphdr *)((char *)t1 +tmp);
	buff->len += tmp;
	memcpy(t1, th, sizeof(*t1));

	/*
	 *	Swap the send and the receive. 
	 */
	// 从收到的tcp头中得到地址
	t1->dest = th->source;
	t1->source = th->dest;
	// 是个重置包
	t1->rst = 1;  
	t1->window = 0;
	// 同时是个ack包
	if(th->ack)
	{
		t1->ack = 0;
	  	t1->seq = th->ack_seq;
	  	t1->ack_seq = 0;
	}
	else
	{
	  	t1->ack = 1;
	  	if(!th->syn)
  			t1->ack_seq=htonl(th->seq);
  		else
  			t1->ack_seq=htonl(th->seq+1);
  		t1->seq=0;
	}

	t1->syn = 0;
	t1->urg = 0;
	t1->fin = 0;
	t1->psh = 0;
	t1->doff = sizeof(*t1)/4;
	tcp_send_check(t1, saddr, daddr, sizeof(*t1), NULL);
	prot->queue_xmit(NULL, ndev, buff, 1);
	tcp_statistics.TcpOutSegs++;
}


/*
 *	Look for tcp options. Parses everything but only knows about MSS.
 *      This routine is always called with the packet containing the SYN.
 *      However it may also be called with the ack to the SYN.  So you
 *      can't assume this is always the SYN.  It's always called after
 *      we have set up sk->mtu to our own MTU.
 *
 *	We need at minimum to add PAWS support here. Possibly large windows
 *	as Linux gets deployed on 100Mb/sec networks.
 */
 
static void tcp_options(struct sock *sk, struct tcphdr *th)
{
	unsigned char *ptr;
	int length=(th->doff*4)-sizeof(struct tcphdr);
	int mss_seen = 0;
    
	ptr = (unsigned char *)(th + 1);
  
	while(length>0)
	{
	  	int opcode=*ptr++;
	  	int opsize=*ptr++;
	  	switch(opcode)
	  	{
	  		case TCPOPT_EOL:
	  			return;
	  		case TCPOPT_NOP:	/* Ref: RFC 793 section 3.1 */
	  			length--;
	  			ptr--;		/* the opsize=*ptr++ above was a mistake */
	  			continue;
	  		
	  		default:
	  			if(opsize<=2)	/* Avoid silly options looping forever */
	  				return;
	  			switch(opcode)
	  			{
	  				case TCPOPT_MSS:
	  					if(opsize==4 && th->syn)
	  					{
	  						sk->mtu=min(sk->mtu,ntohs(*(unsigned short *)ptr));
							mss_seen = 1;
	  					}
	  					break;
		  				/* Add other options here as people feel the urge to implement stuff like large windows */
	  			}
	  			ptr+=opsize-2;
	  			length-=opsize;
	  	}
	}
	if (th->syn) 
	{
		if (! mss_seen)
		      sk->mtu=min(sk->mtu, 536);  /* default MSS if none sent */
	}
#ifdef CONFIG_INET_PCTCP
	sk->mss = min(sk->max_window >> 1, sk->mtu);
#else    
	sk->mss = min(sk->max_window, sk->mtu);
#endif  
}

static inline unsigned long default_mask(unsigned long dst)
{
	dst = ntohl(dst);
	if (IN_CLASSA(dst))
		return htonl(IN_CLASSA_NET);
	if (IN_CLASSB(dst))
		return htonl(IN_CLASSB_NET);
	return htonl(IN_CLASSC_NET);
}

/*
 *	Default sequence number picking algorithm.
 *	As close as possible to RFC 793, which
 *	suggests using a 250kHz clock.
 *	Further reading shows this assumes 2MB/s networks.
 *	For 10MB/s ethernet, a 1MHz clock is appropriate.
 *	That's funny, Linux has one built in!  Use it!
 */

extern inline unsigned long tcp_init_seq(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	return tv.tv_usec+tv.tv_sec*1000000;
}

/*
 *	This routine handles a connection request.
 *	It should make sure we haven't already responded.
 *	Because of the way BSD works, we have to send a syn/ack now.
 *	This also means it will be harder to close a socket which is
 *	listening.
 */
// 收到一个syn包时的处理 
static void tcp_conn_request(struct sock *sk, struct sk_buff *skb,
		 unsigned long daddr, unsigned long saddr,
		 struct options *opt, struct device *dev, unsigned long seq)
{
	struct sk_buff *buff;
	struct tcphdr *t1;
	unsigned char *ptr;
	struct sock *newsk;
	struct tcphdr *th;
	struct device *ndev=NULL;
	int tmp;
	struct rtable *rt;
	
	th = skb->h.th;
	// data_ready是唤醒阻塞在accept函数的进程，而这次还没建立起连接，执行回调没有意义
	/* If the socket is dead, don't accept the connection. */
	if (!sk->dead) 
	{
  		sk->data_ready(sk,0);
	}
	else 
	{	// 该socket已经处于释放状态，发送reset包
		if(sk->debug)
			printk("Reset on %p: Connect on dead socket.\n",sk);
		tcp_reset(daddr, saddr, th, sk->prot, opt, dev, sk->ip_tos,sk->ip_ttl);
		tcp_statistics.TcpAttemptFails++;
		kfree_skb(skb, FREE_READ);
		return;
	}

	/*
	 * Make sure we can accept more.  This will prevent a
	 * flurry of syns from eating up all our memory.
	 */
	// 如果当前的半连接+已连接队列大小大于等于最大值则丢包
	if (sk->ack_backlog >= sk->max_ack_backlog) 
	{
		tcp_statistics.TcpAttemptFails++;
		kfree_skb(skb, FREE_READ);
		return;
	}

	/*
	 * We need to build a new sock struct.
	 * It is sort of bad to have a socket without an inode attached
	 * to it, but the wake_up's will just wake up the listening socket,
	 * and if the listening socket is destroyed before this is taken
	 * off of the queue, this will take care of it.
	 */
	// 分配一个新的sock结构用于连接连接
	newsk = (struct sock *) kmalloc(sizeof(struct sock), GFP_ATOMIC);
	if (newsk == NULL) 
	{
		/* just ignore the syn.  It will get retransmitted. */
		tcp_statistics.TcpAttemptFails++;
		kfree_skb(skb, FREE_READ);
		return;
	}
	// 从listen套接字复制内容，再覆盖某些字段
	memcpy(newsk, sk, sizeof(*newsk));
	skb_queue_head_init(&newsk->write_queue);
	skb_queue_head_init(&newsk->receive_queue);
	newsk->send_head = NULL;
	newsk->send_tail = NULL;
	skb_queue_head_init(&newsk->back_log);
	newsk->rtt = 0;		/*TCP_CONNECT_TIME<<3*/
	newsk->rto = TCP_TIMEOUT_INIT;
	newsk->mdev = 0;
	newsk->max_window = 0;
	newsk->cong_window = 1;
	newsk->cong_count = 0;
	newsk->ssthresh = 0;
	newsk->backoff = 0;
	newsk->blog = 0;
	newsk->intr = 0;
	newsk->proc = 0;
	newsk->done = 0;
	newsk->partial = NULL;
	newsk->pair = NULL;
	newsk->wmem_alloc = 0;
	newsk->rmem_alloc = 0;
	newsk->localroute = sk->localroute;

	newsk->max_unacked = MAX_WINDOW - TCP_WINDOW_DIFF;

	newsk->err = 0;
	newsk->shutdown = 0;
	newsk->ack_backlog = 0;
	// 期待收到的对端下一个字节的序列号
	newsk->acked_seq = skb->h.th->seq+1;
	// 进程可以读但是还没有读取的字节序列号
	newsk->copied_seq = skb->h.th->seq+1;
	// 当收到对端fin包的时候，回复的ack包中的序列号	
	newsk->fin_seq = skb->h.th->seq;
	// 进入syn_recv状态
	newsk->state = TCP_SYN_RECV;
	newsk->timeout = 0;
	newsk->ip_xmit_timeout = 0;
	// 下一个发送的字节的序列号
	newsk->write_seq = seq; 
	// 可发送的字节序列号最大值
	newsk->window_seq = newsk->write_seq;
	// 序列号小于rcv_ack_seq的数据包都已经收到
	newsk->rcv_ack_seq = newsk->write_seq;
	newsk->urg_data = 0;
	newsk->retransmits = 0;
	// 关闭套接字的时候不需要等待一段时间才能关闭
	newsk->linger=0;
	newsk->destroy = 0;
	init_timer(&newsk->timer);
	newsk->timer.data = (unsigned long)newsk;
	newsk->timer.function = &net_timer;
	init_timer(&newsk->retransmit_timer);
	newsk->retransmit_timer.data = (unsigned long)newsk;
	newsk->retransmit_timer.function=&retransmit_timer;
	// 记录ip
	newsk->dummy_th.source = skb->h.th->dest;
	newsk->dummy_th.dest = skb->h.th->source;
	
	/*
	 *	Swap these two, they are from our point of view. 
	 */
	 
	newsk->daddr = saddr;
	newsk->saddr = daddr;
	// 放到tcp的socket哈希队列
	put_sock(newsk->num,newsk);
	newsk->dummy_th.res1 = 0;
	newsk->dummy_th.doff = 6;
	newsk->dummy_th.fin = 0;
	newsk->dummy_th.syn = 0;
	newsk->dummy_th.rst = 0;	
	newsk->dummy_th.psh = 0;
	newsk->dummy_th.ack = 0;
	newsk->dummy_th.urg = 0;
	newsk->dummy_th.res2 = 0;
	newsk->acked_seq = skb->h.th->seq + 1;
	newsk->copied_seq = skb->h.th->seq + 1;
	newsk->socket = NULL;

	/*
	 *	Grab the ttl and tos values and use them 
	 */

	newsk->ip_ttl=sk->ip_ttl;
	newsk->ip_tos=skb->ip_hdr->tos;

	/*
	 *	Use 512 or whatever user asked for 
	 */

	/*
	 * 	Note use of sk->user_mss, since user has no direct access to newsk 
	 */

	rt=ip_rt_route(saddr, NULL,NULL);
	
	if(rt!=NULL && (rt->rt_flags&RTF_WINDOW))
		newsk->window_clamp = rt->rt_window;
	else
		newsk->window_clamp = 0;
		
	if (sk->user_mss)
		newsk->mtu = sk->user_mss;
	else if(rt!=NULL && (rt->rt_flags&RTF_MSS))
		newsk->mtu = rt->rt_mss - HEADER_SIZE;
	else 
	{
#ifdef CONFIG_INET_SNARL	/* Sub Nets Are Local */
		if ((saddr ^ daddr) & default_mask(saddr))
#else
		if ((saddr ^ daddr) & dev->pa_mask)
#endif
			newsk->mtu = 576 - HEADER_SIZE;
		else
			newsk->mtu = MAX_WINDOW;
	}

	/*
	 *	But not bigger than device MTU 
	 */

	newsk->mtu = min(newsk->mtu, dev->mtu - HEADER_SIZE);

	/*
	 *	This will min with what arrived in the packet 
	 */
	// 解析tcp选项
	tcp_options(newsk,skb->h.th);
	// 分配一个skb
	buff = newsk->prot->wmalloc(newsk, MAX_SYN_SIZE, 1, GFP_ATOMIC);
	if (buff == NULL) 
	{
		sk->err = ENOMEM;
		newsk->dead = 1;
		newsk->state = TCP_CLOSE;
		/* And this will destroy it */
		release_sock(newsk);
		kfree_skb(skb, FREE_READ);
		tcp_statistics.TcpAttemptFails++;
		return;
	}
	// skb和sock关联，4个字节是用于tcp mss选项，告诉对端自己的mss
	buff->len = sizeof(struct tcphdr)+4;
	buff->sk = newsk;
	buff->localroute = newsk->localroute;

	t1 =(struct tcphdr *) buff->data;

	/*
	 *	Put in the IP header and routing stuff. 
	 */
	// 构造ip和mac头
	tmp = sk->prot->build_header(buff, newsk->saddr, newsk->daddr, &ndev,
			       IPPROTO_TCP, NULL, MAX_SYN_SIZE,sk->ip_tos,sk->ip_ttl);

	/*
	 *	Something went wrong. 
	 */

	if (tmp < 0) 
	{
		sk->err = tmp;
		buff->free = 1;
		kfree_skb(buff,FREE_WRITE);
		newsk->dead = 1;
		newsk->state = TCP_CLOSE;
		release_sock(newsk);
		skb->sk = sk;
		kfree_skb(skb, FREE_READ);
		tcp_statistics.TcpAttemptFails++;
		return;
	}

	buff->len += tmp;
	// tcp头
	t1 =(struct tcphdr *)((char *)t1 +tmp);
  
	memcpy(t1, skb->h.th, sizeof(*t1));
	buff->h.seq = newsk->write_seq;
	/*
	 *	Swap the send and the receive. 
	 */
	t1->dest = skb->h.th->source;
	t1->source = newsk->dummy_th.source;
	t1->seq = ntohl(newsk->write_seq++);
	// 是个ack包，即第二次握手
	t1->ack = 1;
	newsk->window = tcp_select_window(newsk);
	newsk->sent_seq = newsk->write_seq;
	t1->window = ntohs(newsk->window);
	t1->res1 = 0;
	t1->res2 = 0;
	t1->rst = 0;
	t1->urg = 0;
	t1->psh = 0;
	t1->syn = 1;
	t1->ack_seq = ntohl(skb->h.th->seq+1);
	t1->doff = sizeof(*t1)/4+1;
	ptr =(unsigned char *)(t1+1);
	ptr[0] = 2;
	ptr[1] = 4;
	ptr[2] = ((newsk->mtu) >> 8) & 0xff;
	ptr[3] =(newsk->mtu) & 0xff;

	tcp_send_check(t1, daddr, saddr, sizeof(*t1)+4, newsk);
	// 发送ack，即第二次握手
	newsk->prot->queue_xmit(newsk, ndev, buff, 0);
	reset_xmit_timer(newsk, TIME_WRITE , TCP_TIMEOUT_INIT);
	// skb关联的socket为newsk，accept的时候摘取skb时即拿到该socket返回给应用层
	skb->sk = newsk;

	/*
	 *	Charge the sock_buff to newsk. 
	 */
	// 把skb中数据的大小算在newsk中 
	sk->rmem_alloc -= skb->mem_len;
	newsk->rmem_alloc += skb->mem_len;
	// 
	skb_queue_tail(&sk->receive_queue,skb);
	// 连接队列节点个数加1
	sk->ack_backlog++;
	release_sock(newsk);
	tcp_statistics.TcpOutSegs++;
}

// 关闭一个socket
static void tcp_close(struct sock *sk, int timeout)
{
	/*
	 * We need to grab some memory, and put together a FIN,	
	 * and then put it into the queue to be sent.
	 */
	
	sk->inuse = 1;
	// 监听型的socket要关闭建立的连接
	if(sk->state == TCP_LISTEN)
	{
		/* Special case */
		tcp_set_state(sk, TCP_CLOSE);
		// 关闭已经建立的连接
		tcp_close_pending(sk);
		release_sock(sk);
		return;
	}
	
	sk->keepopen = 1;
	sk->shutdown = SHUTDOWN_MASK;

	if (!sk->dead) 
	  	sk->state_change(sk);

	if (timeout == 0) 
	{
		struct sk_buff *skb;
		
		/*
		 *  We need to flush the recv. buffs.  We do this only on the
		 *  descriptor close, not protocol-sourced closes, because the
		 *  reader process may not have drained the data yet!
		 */
		// 销毁未处理的数据 
		while((skb=skb_dequeue(&sk->receive_queue))!=NULL)
			kfree_skb(skb, FREE_READ);
		/*
		 *	Get rid off any half-completed packets. 
		 */
		// 有小数据包则发送
		if (sk->partial) 
			tcp_send_partial(sk);
	}

		
	/*
	 *	Timeout is not the same thing - however the code likes
	 *	to send both the same way (sigh).
	 */
	 
	if(timeout)
	{
		tcp_set_state(sk, TCP_CLOSE);	/* Dead */
	}
	else
	{
		if(tcp_close_state(sk,1)==1)
		{
			tcp_send_fin(sk);
		}
	}
	release_sock(sk);
}


/*
 * 	This routine takes stuff off of the write queue,
 *	and puts it in the xmit queue. This happens as incoming acks
 *	open up the remote window for us.
 */
 
static void tcp_write_xmit(struct sock *sk)
{
	struct sk_buff *skb;

	/*
	 *	The bytes will have to remain here. In time closedown will
	 *	empty the write queue and all will be happy 
	 */

	if(sk->zapped)
		return;

	/*
	 *	Anything on the transmit queue that fits the window can
	 *	be added providing we are not
	 *
	 *	a) retransmitting (Nagle's rule)
	 *	b) exceeding our congestion window.
	 */
	 
	while((skb = skb_peek(&sk->write_queue)) != NULL &&
		before(skb->h.seq, sk->window_seq + 1) &&
		(sk->retransmits == 0 ||
		 sk->ip_xmit_timeout != TIME_WRITE ||
		 before(skb->h.seq, sk->rcv_ack_seq + 1))
		&& sk->packets_out < sk->cong_window) 
	{
		IS_SKB(skb);
		skb_unlink(skb);
		
		/*
		 *	See if we really need to send the packet. 
		 */
		 
		if (before(skb->h.seq, sk->rcv_ack_seq +1)) 
		{
			/*
			 *	This is acked data. We can discard it. This 
			 *	cannot currently occur.
			 */
			 
			sk->retransmits = 0;
			kfree_skb(skb, FREE_WRITE);
			if (!sk->dead) 
				sk->write_space(sk);
		} 
		else
		{
			struct tcphdr *th;
			struct iphdr *iph;
			int size;
/*
 * put in the ack seq and window at this point rather than earlier,
 * in order to keep them monotonic.  We really want to avoid taking
 * back window allocations.  That's legal, but RFC1122 says it's frowned on.
 * Ack and window will in general have changed since this packet was put
 * on the write queue.
 */
			iph = (struct iphdr *)(skb->data +
					       skb->dev->hard_header_len);
			th = (struct tcphdr *)(((char *)iph) +(iph->ihl << 2));
			size = skb->len - (((unsigned char *) th) - skb->data);
			
			th->ack_seq = ntohl(sk->acked_seq);
			th->window = ntohs(tcp_select_window(sk));

			tcp_send_check(th, sk->saddr, sk->daddr, size, sk);

			sk->sent_seq = skb->h.seq;
			
			/*
			 *	IP manages our queue for some crazy reason
			 */
			 
			sk->prot->queue_xmit(sk, skb->dev, skb, skb->free);
			
			/*
			 *	Again we slide the timer wrongly
			 */
			 
			reset_xmit_timer(sk, TIME_WRITE, sk->rto);
		}
	}
}


/*
 *	This routine deals with incoming acks, but not outgoing ones.
 */

extern __inline__ int tcp_ack(struct sock *sk, struct tcphdr *th, unsigned long saddr, int len)
{
	unsigned long ack;
	int flag = 0;

	/* 
	 * 1 - there was data in packet as well as ack or new data is sent or 
	 *     in shutdown state
	 * 2 - data from retransmit queue was acked and removed
	 * 4 - window shrunk or data from retransmit queue was acked and removed
	 */

	if(sk->zapped)
		return(1);	/* Dead, cant ack any more so why bother */

	/*
	 *	Have we discovered a larger window
	 */
	 
	ack = ntohl(th->ack_seq);
	// 对端的接收窗口大小比之前的大，则更新最大报文的大小
	if (ntohs(th->window) > sk->max_window) 
	{
  		sk->max_window = ntohs(th->window);
#ifdef CONFIG_INET_PCTCP
		/* Hack because we don't send partial packets to non SWS
		   handling hosts */
		sk->mss = min(sk->max_window>>1, sk->mtu);
#else
		// 更新mss，取最大报文大小和mtu的最小值
		sk->mss = min(sk->max_window, sk->mtu);
#endif	
	}

	/*
	 *	We have dropped back to keepalive timeouts. Thus we have
	 *	no retransmits pending.
	 */
	 
	if (sk->retransmits && sk->ip_xmit_timeout == TIME_KEEPOPEN)
	  	sk->retransmits = 0;

	/*
	 *	If the ack is newer than sent or older than previous acks
	 *	then we can probably ignore it.
	 */
	// ack序号比下一个需要发送的字节序号还大或者比已经确认过的序列号还小 
	if (after(ack, sk->sent_seq) || before(ack, sk->rcv_ack_seq)) 
	{
		if(sk->debug)
			printk("Ack ignored %lu %lu\n",ack,sk->sent_seq);
			
		/*
		 *	Keepalive processing.
		 */
		// 无效
		if (after(ack, sk->sent_seq)) 
		{
			return(0);
		}
		
		/*
		 *	Restart the keepalive timer.
		 */
		// 开启了长连接 
		if (sk->keepopen) 
		{
			if(sk->ip_xmit_timeout==TIME_KEEPOPEN)
				// 重置断开连接的超时时间
				reset_xmit_timer(sk, TIME_KEEPOPEN, TCP_TIMEOUT_LEN);
		}
		return(1);
	}

	/*
	 *	If there is data set flag 1
	 */
	// tcp头部长度
	if (len != th->doff*4) 
		flag |= 1;

	/*
	 *	See if our window has been shrunk. 
	 */
	// 当前能发送的最大序列号，已经收到的最大序列号+还能接收的大小
	if (after(sk->window_seq, ack+ntohs(th->window))) 
	{
		/*
		 * We may need to move packets from the send queue
		 * to the write queue, if the window has been shrunk on us.
		 * The RFC says you are not allowed to shrink your window
		 * like this, but if the other end does, you must be able
		 * to deal with it.
		 */
		struct sk_buff *skb;
		struct sk_buff *skb2;
		struct sk_buff *wskb = NULL;
		// 发送还没有收到确认的skb队列
		skb2 = sk->send_head;
		sk->send_head = NULL;
		sk->send_tail = NULL;
	
		/*
		 *	This is an artifact of a flawed concept. We want one
		 *	queue and a smarter send routine when we send all.
		 */
	
		flag |= 4;	/* Window changed */
		// 更新可以发送的序列号最大值
		sk->window_seq = ack + ntohs(th->window);
		cli();
		while (skb2 != NULL) 
		{
			// skb指向队列中的第一个skb，skb2指向第一个skb的下一个
			skb = skb2;
			skb2 = skb->link3;
			skb->link3 = NULL;
			// 当前发送的所有数据都已经收到确认了
			if (after(skb->h.seq, sk->window_seq)) 
			{	
				// 已经发送但还没有收到确认的数据包个数减一
				if (sk->packets_out > 0) 
					sk->packets_out--;
				/* We may need to remove this from the dev send list. */
				// 删除该skb
				if (skb->next != NULL) 
				{
					skb_unlink(skb);				
				}
				/* Now add it to the write_queue. */
				if (wskb == NULL)
					skb_queue_head(&sk->write_queue,skb);
				else
					skb_append(wskb,skb);
				wskb = skb;
			} 
			else 
			{
				if (sk->send_head == NULL) 
				{
					sk->send_head = skb;
					sk->send_tail = skb;
				}
				else
				{
					sk->send_tail->link3 = skb;
					sk->send_tail = skb;
				}
				skb->link3 = NULL;
			}
		}
		sti();
	}

	/*
	 *	Pipe has emptied
	 */
	 
	if (sk->send_tail == NULL || sk->send_head == NULL) 
	{
		sk->send_head = NULL;
		sk->send_tail = NULL;
		sk->packets_out= 0;
	}

	/*
	 *	Update the right hand window edge of the host
	 */
	 
	sk->window_seq = ack + ntohs(th->window);

	/*
	 *	We don't want too many packets out there. 
	 */
	 
	if (sk->ip_xmit_timeout == TIME_WRITE && 
		sk->cong_window < 2048 && after(ack, sk->rcv_ack_seq)) 
	{
		/* 
		 * This is Jacobson's slow start and congestion avoidance. 
		 * SIGCOMM '88, p. 328.  Because we keep cong_window in integral
		 * mss's, we can't do cwnd += 1 / cwnd.  Instead, maintain a 
		 * counter and increment it once every cwnd times.  It's possible
		 * that this should be done only if sk->retransmits == 0.  I'm
		 * interpreting "new data is acked" as including data that has
		 * been retransmitted but is just now being acked.
		 */
		if (sk->cong_window < sk->ssthresh)  
			/* 
			 *	In "safe" area, increase
			 */
			sk->cong_window++;
		else 
		{
			/*
			 *	In dangerous area, increase slowly.  In theory this is
			 *  	sk->cong_window += 1 / sk->cong_window
			 */
			if (sk->cong_count >= sk->cong_window) 
			{
				sk->cong_window++;
				sk->cong_count = 0;
			}
			else 
				sk->cong_count++;
		}
	}

	/*
	 *	Remember the highest ack received.
	 */
	 
	sk->rcv_ack_seq = ack;

	/*
	 *	If this ack opens up a zero window, clear backoff.  It was
	 *	being used to time the probes, and is probably far higher than
	 *	it needs to be for normal retransmission.
	 */

	if (sk->ip_xmit_timeout == TIME_PROBE0) 
	{
		sk->retransmits = 0;	/* Our probe was answered */
		
		/*
		 *	Was it a usable window open ?
		 */
		 
  		if (skb_peek(&sk->write_queue) != NULL &&   /* should always be non-null */
		    ! before (sk->window_seq, sk->write_queue.next->h.seq)) 
		{
			sk->backoff = 0;
			
			/*
			 *	Recompute rto from rtt.  this eliminates any backoff.
			 */

			sk->rto = ((sk->rtt >> 2) + sk->mdev) >> 1;
			if (sk->rto > 120*HZ)
				sk->rto = 120*HZ;
			if (sk->rto < 20)	/* Was 1*HZ, then 1 - turns out we must allow about
						   .2 of a second because of BSD delayed acks - on a 100Mb/sec link
						   .2 of a second is going to need huge windows (SIGH) */
			sk->rto = 20;
		}
	}

	/* 
	 *	See if we can take anything off of the retransmit queue.
	 */
   
	while(sk->send_head != NULL) 
	{
		/* Check for a bug. */
		if (sk->send_head->link3 &&
		    after(sk->send_head->h.seq, sk->send_head->link3->h.seq)) 
			printk("INET: tcp.c: *** bug send_list out of order.\n");
			
		/*
		 *	If our packet is before the ack sequence we can
		 *	discard it as it's confirmed to have arrived the other end.
		 */
		 
		if (before(sk->send_head->h.seq, ack+1)) 
		{
			struct sk_buff *oskb;	
			if (sk->retransmits) 
			{	
				/*
				 *	We were retransmitting.  don't count this in RTT est 
				 */
				flag |= 2;

				/*
				 * even though we've gotten an ack, we're still
				 * retransmitting as long as we're sending from
				 * the retransmit queue.  Keeping retransmits non-zero
				 * prevents us from getting new data interspersed with
				 * retransmissions.
				 */

				if (sk->send_head->link3)	/* Any more queued retransmits? */
					sk->retransmits = 1;
				else
					sk->retransmits = 0;
			}
  			/*
			 * Note that we only reset backoff and rto in the
			 * rtt recomputation code.  And that doesn't happen
			 * if there were retransmissions in effect.  So the
			 * first new packet after the retransmissions is
			 * sent with the backoff still in effect.  Not until
			 * we get an ack from a non-retransmitted packet do
			 * we reset the backoff and rto.  This allows us to deal
			 * with a situation where the network delay has increased
			 * suddenly.  I.e. Karn's algorithm. (SIGCOMM '87, p5.)
			 */

			/*
			 *	We have one less packet out there. 
			 */
			 
			if (sk->packets_out > 0) 
				sk->packets_out --;
			/* 
			 *	Wake up the process, it can probably write more. 
			 */
			if (!sk->dead) 
				sk->write_space(sk);
			oskb = sk->send_head;

			if (!(flag&2)) 	/* Not retransmitting */
			{
				long m;
	
				/*
				 *	The following amusing code comes from Jacobson's
				 *	article in SIGCOMM '88.  Note that rtt and mdev
				 *	are scaled versions of rtt and mean deviation.
				 *	This is designed to be as fast as possible 
				 *	m stands for "measurement".
				 */
	
				m = jiffies - oskb->when;  /* RTT */
				if(m<=0)
					m=1;		/* IS THIS RIGHT FOR <0 ??? */
				m -= (sk->rtt >> 3);    /* m is now error in rtt est */
				sk->rtt += m;           /* rtt = 7/8 rtt + 1/8 new */
				if (m < 0)
					m = -m;		/* m is now abs(error) */
				m -= (sk->mdev >> 2);   /* similar update on mdev */
				sk->mdev += m;	    	/* mdev = 3/4 mdev + 1/4 new */
	
				/*
				 *	Now update timeout.  Note that this removes any backoff.
				 */
			 
				sk->rto = ((sk->rtt >> 2) + sk->mdev) >> 1;
				if (sk->rto > 120*HZ)
					sk->rto = 120*HZ;
				if (sk->rto < 20)	/* Was 1*HZ - keep .2 as minimum cos of the BSD delayed acks */
					sk->rto = 20;
				sk->backoff = 0;
			}
			flag |= (2|4);	/* 2 is really more like 'don't adjust the rtt 
			                   In this case as we just set it up */
			cli();
			oskb = sk->send_head;
			IS_SKB(oskb);
			sk->send_head = oskb->link3;
			if (sk->send_head == NULL) 
			{
				sk->send_tail = NULL;
			}

		/*
		 *	We may need to remove this from the dev send list. 
		 */

			if (oskb->next)
				skb_unlink(oskb);
			sti();
			kfree_skb(oskb, FREE_WRITE); /* write. */
			if (!sk->dead) 
				sk->write_space(sk);
		}
		else
		{
			break;
		}
	}

	/*
	 * XXX someone ought to look at this too.. at the moment, if skb_peek()
	 * returns non-NULL, we complete ignore the timer stuff in the else
	 * clause.  We ought to organize the code so that else clause can
	 * (should) be executed regardless, possibly moving the PROBE timer
	 * reset over.  The skb_peek() thing should only move stuff to the
	 * write queue, NOT also manage the timer functions.
	 */

	/*
	 * Maybe we can take some stuff off of the write queue,
	 * and put it onto the xmit queue.
	 */
	if (skb_peek(&sk->write_queue) != NULL) 
	{
		if (after (sk->window_seq+1, sk->write_queue.next->h.seq) &&
		        (sk->retransmits == 0 || 
			 sk->ip_xmit_timeout != TIME_WRITE ||
			 before(sk->write_queue.next->h.seq, sk->rcv_ack_seq + 1))
			&& sk->packets_out < sk->cong_window) 
		{
			/*
			 *	Add more data to the send queue.
			 */
			flag |= 1;
			tcp_write_xmit(sk);
		}
		else if (before(sk->window_seq, sk->write_queue.next->h.seq) &&
 			sk->send_head == NULL &&
 			sk->ack_backlog == 0 &&
 			sk->state != TCP_TIME_WAIT) 
 		{
 			/*
 			 *	Data to queue but no room.
 			 */
 	        	reset_xmit_timer(sk, TIME_PROBE0, sk->rto);
 		}		
	}
	else
	{
		/*
		 * from TIME_WAIT we stay in TIME_WAIT as long as we rx packets
		 * from TCP_CLOSE we don't do anything
		 *
		 * from anything else, if there is write data (or fin) pending,
		 * we use a TIME_WRITE timeout, else if keepalive we reset to
		 * a KEEPALIVE timeout, else we delete the timer.
		 *
		 * We do not set flag for nominal write data, otherwise we may
		 * force a state where we start to write itsy bitsy tidbits
		 * of data.
		 */

		switch(sk->state) {
		case TCP_TIME_WAIT:
			/*
			 * keep us in TIME_WAIT until we stop getting packets,
			 * reset the timeout.
			 */
			reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
			break;
		case TCP_CLOSE:
			/*
			 * don't touch the timer.
			 */
			break;
		default:
			/*
			 * 	Must check send_head, write_queue, and ack_backlog
			 * 	to determine which timeout to use.
			 */
			if (sk->send_head || skb_peek(&sk->write_queue) != NULL || sk->ack_backlog) {
				reset_xmit_timer(sk, TIME_WRITE, sk->rto);
			} else if (sk->keepopen) {
				reset_xmit_timer(sk, TIME_KEEPOPEN, TCP_TIMEOUT_LEN);
			} else {
				del_timer(&sk->retransmit_timer);
				sk->ip_xmit_timeout = 0;
			}
			break;
		}
	}

	/*
	 *	We have nothing queued but space to send. Send any partial
	 *	packets immediately (end of Nagle rule application).
	 */
	 
	if (sk->packets_out == 0 && sk->partial != NULL &&
		skb_peek(&sk->write_queue) == NULL && sk->send_head == NULL) 
	{
		flag |= 1;
		tcp_send_partial(sk);
	}

	/*
	 * In the LAST_ACK case, the other end FIN'd us.  We then FIN'd them, and
	 * we are now waiting for an acknowledge to our FIN.  The other end is
	 * already in TIME_WAIT.
	 *
	 * Move to TCP_CLOSE on success.
	 */

	if (sk->state == TCP_LAST_ACK) 
	{
		if (!sk->dead)
			sk->state_change(sk);
		if(sk->debug)
			printk("rcv_ack_seq: %lX==%lX, acked_seq: %lX==%lX\n",
				sk->rcv_ack_seq,sk->write_seq,sk->acked_seq,sk->fin_seq);
		if (sk->rcv_ack_seq == sk->write_seq /*&& sk->acked_seq == sk->fin_seq*/) 
		{
			flag |= 1;
			tcp_set_state(sk,TCP_CLOSE);
			sk->shutdown = SHUTDOWN_MASK;
		}
	}

	/*
	 *	Incoming ACK to a FIN we sent in the case of our initiating the close.
	 *
	 *	Move to FIN_WAIT2 to await a FIN from the other end. Set
	 *	SEND_SHUTDOWN but not RCV_SHUTDOWN as data can still be coming in.
	 */

	if (sk->state == TCP_FIN_WAIT1) 
	{

		if (!sk->dead) 
			sk->state_change(sk);
		// 对端已经收到本端的数据的序列号等于下一个应用层数据的序列号，说明本端的数据发送完毕
		if (sk->rcv_ack_seq == sk->write_seq) 
		{
			flag |= 1;
			sk->shutdown |= SEND_SHUTDOWN;
			tcp_set_state(sk, TCP_FIN_WAIT2);
		}
	}

	/*
	 *	Incoming ACK to a FIN we sent in the case of a simultaneous close.
	 *
	 *	Move to TIME_WAIT
	 */

	if (sk->state == TCP_CLOSING) 
	{

		if (!sk->dead) 
			sk->state_change(sk);
		if (sk->rcv_ack_seq == sk->write_seq) 
		{
			flag |= 1;
			tcp_time_wait(sk);
		}
	}
	
	/*
	 *	Final ack of a three way shake 
	 */
	 
	if(sk->state==TCP_SYN_RECV)
	{
		tcp_set_state(sk, TCP_ESTABLISHED);
		tcp_options(sk,th);
		sk->dummy_th.dest=th->source;
		sk->copied_seq = sk->acked_seq;
		// 唤醒阻塞在等待连接建立的进程
		if(!sk->dead)
			sk->state_change(sk);
		if(sk->max_window==0)
		{
			sk->max_window=32;	/* Sanity check */
			sk->mss=min(sk->max_window,sk->mtu);
		}
	}
	
	/*
	 * I make no guarantees about the first clause in the following
	 * test, i.e. "(!flag) || (flag&4)".  I'm not entirely sure under
	 * what conditions "!flag" would be true.  However I think the rest
	 * of the conditions would prevent that from causing any
	 * unnecessary retransmission. 
	 *   Clearly if the first packet has expired it should be 
	 * retransmitted.  The other alternative, "flag&2 && retransmits", is
	 * harder to explain:  You have to look carefully at how and when the
	 * timer is set and with what timeout.  The most recent transmission always
	 * sets the timer.  So in general if the most recent thing has timed
	 * out, everything before it has as well.  So we want to go ahead and
	 * retransmit some more.  If we didn't explicitly test for this
	 * condition with "flag&2 && retransmits", chances are "when + rto < jiffies"
	 * would not be true.  If you look at the pattern of timing, you can
	 * show that rto is increased fast enough that the next packet would
	 * almost never be retransmitted immediately.  Then you'd end up
	 * waiting for a timeout to send each packet on the retransmission
	 * queue.  With my implementation of the Karn sampling algorithm,
	 * the timeout would double each time.  The net result is that it would
	 * take a hideous amount of time to recover from a single dropped packet.
	 * It's possible that there should also be a test for TIME_WRITE, but
	 * I think as long as "send_head != NULL" and "retransmit" is on, we've
	 * got to be in real retransmission mode.
	 *   Note that tcp_do_retransmit is called with all==1.  Setting cong_window
	 * back to 1 at the timeout will cause us to send 1, then 2, etc. packets.
	 * As long as no further losses occur, this seems reasonable.
	 */
	
	if (((!flag) || (flag&4)) && sk->send_head != NULL &&
	       (((flag&2) && sk->retransmits) ||
	       (sk->send_head->when + sk->rto < jiffies))) 
	{
		if(sk->send_head->when + sk->rto < jiffies)
			tcp_retransmit(sk,0);	
		else
		{
			tcp_do_retransmit(sk, 1);
			reset_xmit_timer(sk, TIME_WRITE, sk->rto);
		}
	}

	return(1);
}


/*
 * 	Process the FIN bit. This now behaves as it is supposed to work
 *	and the FIN takes effect when it is validly part of sequence
 *	space. Not before when we get holes.
 *
 *	If we are ESTABLISHED, a received fin moves us to CLOSE-WAIT
 *	(and thence onto LAST-ACK and finally, CLOSE, we never enter
 *	TIME-WAIT)
 *
 *	If we are in FINWAIT-1, a received FIN indicates simultaneous
 *	close and we go into CLOSING (and later onto TIME-WAIT)
 *
 *	If we are in FINWAIT-2, a received FIN moves us to TIME-WAIT.
 *
 */
 
static int tcp_fin(struct sk_buff *skb, struct sock *sk, struct tcphdr *th)
{
	sk->fin_seq = th->seq + skb->len + th->syn + th->fin;

	if (!sk->dead) 
	{
		sk->state_change(sk);
		sock_wake_async(sk->socket, 1);
	}

	switch(sk->state) 
	{	
		case TCP_SYN_RECV:
		case TCP_SYN_SENT:
		case TCP_ESTABLISHED:
			/*
			 * move to CLOSE_WAIT, tcp_data() already handled
			 * sending the ack.
			 */
			tcp_set_state(sk,TCP_CLOSE_WAIT);
			if (th->rst)
				sk->shutdown = SHUTDOWN_MASK;
			break;

		case TCP_CLOSE_WAIT:
		case TCP_CLOSING:
			/*
			 * received a retransmission of the FIN, do
			 * nothing.
			 */
			break;
		case TCP_TIME_WAIT:
			/*
			 * received a retransmission of the FIN,
			 * restart the TIME_WAIT timer.
			 */
			reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
			return(0);
		case TCP_FIN_WAIT1:
			/*
			 * This case occurs when a simultaneous close
			 * happens, we must ack the received FIN and
			 * enter the CLOSING state.
			 *
			 * This causes a WRITE timeout, which will either
			 * move on to TIME_WAIT when we timeout, or resend
			 * the FIN properly (maybe we get rid of that annoying
			 * FIN lost hang). The TIME_WRITE code is already correct
			 * for handling this timeout.
			 */

			if(sk->ip_xmit_timeout != TIME_WRITE)
				reset_xmit_timer(sk, TIME_WRITE, sk->rto);
			tcp_set_state(sk,TCP_CLOSING);
			break;
		case TCP_FIN_WAIT2:
			/*
			 * received a FIN -- send ACK and enter TIME_WAIT
			 */
			reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
			sk->shutdown|=SHUTDOWN_MASK;
			tcp_set_state(sk,TCP_TIME_WAIT);
			break;
		case TCP_CLOSE:
			/*
			 * already in CLOSE
			 */
			break;
		default:
			tcp_set_state(sk,TCP_LAST_ACK);
	
			/* Start the timers. */
			reset_msl_timer(sk, TIME_CLOSE, TCP_TIMEWAIT_LEN);
			return(0);
	}

	return(0);
}



/*
 *	This routine handles the data.  If there is room in the buffer,
 *	it will be have already been moved into it.  If there is no
 *	room, then we will just have to discard the packet.
 */

extern __inline__ int tcp_data(struct sk_buff *skb, struct sock *sk, 
	 unsigned long saddr, unsigned short len)
{
	struct sk_buff *skb1, *skb2;
	struct tcphdr *th;
	int dup_dumped=0;
	unsigned long new_seq;
	unsigned long shut_seq;

	th = skb->h.th;
	// 有效数据长度
	skb->len = len -(th->doff*4);

	/*
	 *	The bytes in the receive read/assembly queue has increased. Needed for the
	 *	low memory discard algorithm 
	 */
	// 接收到的数据总大小	   
	sk->bytes_rcv += skb->len;
	// 这个包没有数据并且不是fin包，即一个探测包
	if (skb->len == 0 && !th->fin) 
	{
		/* 
		 *	Don't want to keep passing ack's back and forth. 
		 *	(someone sent us dataless, boring frame)
		 */
		if (!th->ack)
			tcp_send_ack(sk->sent_seq, sk->acked_seq,sk, th, saddr);
		kfree_skb(skb, FREE_READ);
		return(0);
	}
	
	/*
	 *	We no longer have anyone receiving data on this connection.
	 */

#ifndef TCP_DONT_RST_SHUTDOWN		 
	// 本端发送数据端已经关闭，
	if(sk->shutdown & RCV_SHUTDOWN)
	{
		/*
		 *	FIXME: BSD has some magic to avoid sending resets to
		 *	broken 4.2 BSD keepalives. Much to my surprise a few non
		 *	BSD stacks still have broken keepalives so we want to
		 *	cope with it.
		 */

		if(skb->len)	/* We don't care if it's just an ack or
				   a keepalive/window probe */
		{
			new_seq= th->seq + skb->len + th->syn;	/* Right edge of _data_ part of frame */
			
			/* Do this the way 4.4BSD treats it. Not what I'd
			   regard as the meaning of the spec but it's what BSD
			   does and clearly they know everything 8) */

			/*
			 *	This is valid because of two things
			 *
			 *	a) The way tcp_data behaves at the bottom.
			 *	b) A fin takes effect when read not when received.
			 */
			 
			shut_seq=sk->acked_seq+1;	/* Last byte */
			
			if(after(new_seq,shut_seq))
			{
				if(sk->debug)
					printk("Data arrived on %p after close [Data right edge %lX, Socket shut on %lX] %d\n",
						sk, new_seq, shut_seq, sk->blog);
				if(sk->dead)
				{
					sk->acked_seq = new_seq + th->fin;
					tcp_reset(sk->saddr, sk->daddr, skb->h.th,
						sk->prot, NULL, skb->dev, sk->ip_tos, sk->ip_ttl);
					tcp_statistics.TcpEstabResets++;
					tcp_set_state(sk,TCP_CLOSE);
					sk->err = EPIPE;
					sk->shutdown = SHUTDOWN_MASK;
					kfree_skb(skb, FREE_READ);
					return 0;
				}
			}
		}
	}

#endif

	/*
	 * 	Now we have to walk the chain, and figure out where this one
	 * 	goes into it.  This is set up so that the last packet we received
	 * 	will be the first one we look at, that way if everything comes
	 * 	in order, there will be no performance loss, and if they come
	 * 	out of order we will be able to fit things in nicely.
	 *
	 *	[AC: This is wrong. We should assume in order first and then walk
	 *	 forwards from the first hole based upon real traffic patterns.]
	 *	
	 */
	// 接收队列是空则直接赋值成为第一个节点，否则插入接收队列的相应位置
	if (skb_peek(&sk->receive_queue) == NULL) 	/* Empty queue is easy case */
	{
		skb_queue_head(&sk->receive_queue,skb);
		skb1= NULL;
	} 
	else
	{
		for(skb1=sk->receive_queue.prev; ; skb1 = skb1->prev) 
		{
			if(sk->debug)
			{
				printk("skb1=%p :", skb1);
				printk("skb1->h.th->seq = %ld: ", skb1->h.th->seq);
				printk("skb->h.th->seq = %ld\n",skb->h.th->seq);
				printk("copied_seq = %ld acked_seq = %ld\n", sk->copied_seq,
						sk->acked_seq);
			}
			
			/*
			 *	Optimisation: Duplicate frame or extension of previous frame from
			 *	same sequence point (lost ack case).
			 *	The frame contains duplicate data or replaces a previous frame
			 *	discard the previous frame (safe as sk->inuse is set) and put
			 *	the new one in its place.
			 */
			// 和某个节点的序列号一样但是数据比他多，则删除旧的skb，插入新的skb
			if (th->seq==skb1->h.th->seq && skb->len>= skb1->len)
			{	
				skb_append(skb1,skb);
				skb_unlink(skb1);
				kfree_skb(skb1,FREE_READ);
				dup_dumped=1;
				skb1=NULL;
				break;
			}
			
			/*
			 *	Found where it fits
			 */
			 
			if (after(th->seq+1, skb1->h.th->seq))
			{
				skb_append(skb1,skb);
				break;
			}
			
			/*
			 *	See if we've hit the start. If so insert.
			 */
			if (skb1 == skb_peek(&sk->receive_queue))
			{
				skb_queue_head(&sk->receive_queue, skb);
				break;
			}
		}
  	}

	/*
	 *	Figure out what the ack value for this frame is
	 */
	// ack等于收到的序列号+数据长度 
 	th->ack_seq = th->seq + skb->len;
	// syn和fin都占一个序列号
 	if (th->syn) 
 		th->ack_seq++;
 	if (th->fin)
 		th->ack_seq++;

	if (before(sk->acked_seq, sk->copied_seq)) 
	{
		printk("*** tcp.c:tcp_data bug acked < copied\n");
		sk->acked_seq = sk->copied_seq;
	}

	/*
	 *	Now figure out if we can ack anything. This is very messy because we really want two
	 *	receive queues, a completed and an assembly queue. We also want only one transmit
	 *	queue.
	 */

	if ((!dup_dumped && (skb1 == NULL || skb1->acked)) || before(th->seq, sk->acked_seq+1)) 
	{
		if (before(th->seq, sk->acked_seq+1)) 
		{
			int newwindow;

			if (after(th->ack_seq, sk->acked_seq)) 
			{
				newwindow = sk->window-(th->ack_seq - sk->acked_seq);
				if (newwindow < 0)
					newwindow = 0;	
				sk->window = newwindow;
				sk->acked_seq = th->ack_seq;
			}
			skb->acked = 1;

			/*
			 *	When we ack the fin, we do the FIN 
			 *	processing.
			 */
			// 收到的是fin包
			if (skb->h.th->fin) 
			{
				tcp_fin(skb,sk,skb->h.th);
			}
	  
			for(skb2 = skb->next;
			    skb2 != (struct sk_buff *)&sk->receive_queue;
			    skb2 = skb2->next) 
			{
				if (before(skb2->h.th->seq, sk->acked_seq+1)) 
				{
					if (after(skb2->h.th->ack_seq, sk->acked_seq))
					{
						newwindow = sk->window -
						 (skb2->h.th->ack_seq - sk->acked_seq);
						if (newwindow < 0)
							newwindow = 0;	
						sk->window = newwindow;
						sk->acked_seq = skb2->h.th->ack_seq;
					}
					skb2->acked = 1;
					/*
					 * 	When we ack the fin, we do
					 * 	the fin handling.
					 */
					if (skb2->h.th->fin) 
					{
						tcp_fin(skb,sk,skb->h.th);
					}

					/*
					 *	Force an immediate ack.
					 */
					 
					sk->ack_backlog = sk->max_ack_backlog;
				}
				else
				{
					break;
				}
			}

			/*
			 *	This also takes care of updating the window.
			 *	This if statement needs to be simplified.
			 */
			if (!sk->delay_acks ||
			    sk->ack_backlog >= sk->max_ack_backlog || 
			    sk->bytes_rcv > sk->max_unacked || th->fin) {
	/*			tcp_send_ack(sk->sent_seq, sk->acked_seq,sk,th, saddr); */
			}
			else 
			{
				sk->ack_backlog++;
				if(sk->debug)
					printk("Ack queued.\n");
				reset_xmit_timer(sk, TIME_WRITE, TCP_ACK_TIME);
			}
		}
	}

	/*
	 *	If we've missed a packet, send an ack.
	 *	Also start a timer to send another.
	 */
	 
	if (!skb->acked) 
	{
	
	/*
	 *	This is important.  If we don't have much room left,
	 *	we need to throw out a few packets so we have a good
	 *	window.  Note that mtu is used, not mss, because mss is really
	 *	for the send side.  He could be sending us stuff as large as mtu.
	 */
		 
		while (sk->prot->rspace(sk) < sk->mtu) 
		{
			skb1 = skb_peek(&sk->receive_queue);
			if (skb1 == NULL) 
			{
				printk("INET: tcp.c:tcp_data memory leak detected.\n");
				break;
			}

			/*
			 *	Don't throw out something that has been acked. 
			 */
		 
			if (skb1->acked) 
			{
				break;
			}
		
			skb_unlink(skb1);
			kfree_skb(skb1, FREE_READ);
		}
		tcp_send_ack(sk->sent_seq, sk->acked_seq, sk, th, saddr);
		sk->ack_backlog++;
		reset_xmit_timer(sk, TIME_WRITE, TCP_ACK_TIME);
	}
	else
	{
		tcp_send_ack(sk->sent_seq, sk->acked_seq, sk, th, saddr);
	}

	/*
	 *	Now tell the user we may have some data. 
	 */
	 
	if (!sk->dead) 
	{
        	if(sk->debug)
        		printk("Data wakeup.\n");
		sk->data_ready(sk,0);
	} 
	return(0);
}


/*
 *	This routine is only called when we have urgent data
 *	signalled. Its the 'slow' part of tcp_urg. It could be
 *	moved inline now as tcp_urg is only called from one
 *	place. We handle URGent data wrong. We have to - as
 *	BSD still doesn't use the correction from RFC961.
 */
 
static void tcp_check_urg(struct sock * sk, struct tcphdr * th)
{	
	// 指向紧急数据最后一个字节的下一个字节
	unsigned long ptr = ntohs(th->urg_ptr);
	// ptr指向有效数据的最后一个字节，
	if (ptr)
		ptr--;
	// 数据的第一个字节的序列号+偏移,ptr指向紧急数据偏移
	ptr += th->seq;

	/* ignore urgent data that we've already seen and read */
	// ptr小于可以已读取的字节的序列号，说明ptr指向的数据被读取过了
	if (after(sk->copied_seq, ptr))
		return;

	/* do we already have a newer (or duplicate) urgent pointer? */
	// 之前已经收到过紧急数据，并且之前收到的紧急数据序列号比现在收到的大
	if (sk->urg_data && !after(ptr, sk->urg_seq))
		return;

	/* tell the world about our new urgent pointer */
	// 通知进程或组收到紧急数据
	if (sk->proc != 0) {
		if (sk->proc > 0) {
			// 给进程发送一个SIGURG信号
			kill_proc(sk->proc, SIGURG, 1);
		} else {
			// 给进程组发送一个SIGURG信号
			kill_pg(-sk->proc, SIGURG, 1);
		}
	}
	// 标记紧急数据有效
	sk->urg_data = URG_NOTYET;
	// 设置紧急数据的序列号
	sk->urg_seq = ptr;
}

/*
 *	This is the 'fast' part of urgent handling.
 */
 
extern __inline__ int tcp_urg(struct sock *sk, struct tcphdr *th,
	unsigned long saddr, unsigned long len)
{
	unsigned long ptr;

	/*
	 *	Check if we get a new urgent pointer - normally not 
	 */
	// 报文设置了紧急标记位，说明有紧急数据需要处理 
	if (th->urg)
		tcp_check_urg(sk,th);

	/*
	 *	Do we wait for any urgent data? - normally not
	 */
	// 在tcp_check_urg里设置，说明紧急数据有效，紧急数据可被当做普通数据处理。
	if (sk->urg_data != URG_NOTYET)
		return 0;

	/*
	 *	Is the urgent pointer pointing into this packet? 
	 */
	// 指向紧急数据相对偏移
	ptr = sk->urg_seq - th->seq + th->doff*4;
	if (ptr >= len)
		return 0;

	/*
	 *	Ok, got the correct packet, update info 
	 */
	// urg_data是两个字节，一个保存标记，一个保存一个字节的紧急数据
	sk->urg_data = URG_VALID | *(ptr + (unsigned char *) th);
	if (!sk->dead)
		sk->data_ready(sk,0);
	return 0;
}

/*
 *	This will accept the next outstanding connection. 
 */
 
static struct sock *tcp_accept(struct sock *sk, int flags)
{
	struct sock *newsk;
	struct sk_buff *skb;
  
  /*
   * We need to make sure that this socket is listening,
   * and that it has something pending.
   */
	// 是一个listen的套接字
	if (sk->state != TCP_LISTEN) 
	{
		sk->err = EINVAL;
		return(NULL); 
	}

	/* Avoid the race. */
	cli();
	sk->inuse = 1;
	// 从sock的receive_queue队列摘取已建立连接的节点，
	while((skb = tcp_dequeue_established(sk)) == NULL) 
	{	
		// 没有已经建立连接的节点，但是设置了非阻塞模式，直接返回
		if (flags & O_NONBLOCK) 
		{
			sti();
			release_sock(sk);
			sk->err = EAGAIN;
			return(NULL);
		}

		release_sock(sk);
		//阻塞进程，如果后续建立了连接，则进程被唤醒的时候，就会跳出while循环
		interruptible_sleep_on(sk->sleep);
		if (current->signal & ~current->blocked) 
		{
			sti();
			sk->err = ERESTARTSYS;
			return(NULL);
		}
		sk->inuse = 1;
  	}
	sti();

	/*
	 *	Now all we need to do is return skb->sk. 
	 */
	// 拿到一个新的sock结构，由建立连接的时候创建的
	newsk = skb->sk;

	kfree_skb(skb, FREE_READ);
	// 队列节点数减一
	sk->ack_backlog--;
	release_sock(sk);
	// 返回新的sock结构体
	return(newsk);
}


/*
 *	This will initiate an outgoing connection. 
 */
 
static int tcp_connect(struct sock *sk, struct sockaddr_in *usin, int addr_len)
{
	struct sk_buff *buff;
	struct device *dev=NULL;
	unsigned char *ptr;
	int tmp;
	int atype;
	struct tcphdr *t1;
	struct rtable *rt;

	if (sk->state != TCP_CLOSE) 
	{
		return(-EISCONN);
	}
	
	if (addr_len < 8) 
		return(-EINVAL);

	if (usin->sin_family && usin->sin_family != AF_INET) 
		return(-EAFNOSUPPORT);

  	/*
  	 *	connect() to INADDR_ANY means loopback (BSD'ism).
  	 */
  	// 不传ip则取本机ip
  	if(usin->sin_addr.s_addr==INADDR_ANY)
		usin->sin_addr.s_addr=ip_my_addr();
		  
	/*
	 *	Don't want a TCP connection going to a broadcast address 
	 */
	// 禁止广播和多播
	if ((atype=ip_chk_addr(usin->sin_addr.s_addr)) == IS_BROADCAST || atype==IS_MULTICAST) 
		return -ENETUNREACH;
  
	sk->inuse = 1;
	// 连接的远端地址
	sk->daddr = usin->sin_addr.s_addr;
	// 第一个字节的序列号
	sk->write_seq = tcp_init_seq();
	sk->window_seq = sk->write_seq;
	sk->rcv_ack_seq = sk->write_seq -1;
	sk->err = 0;
	// 远端端口
	sk->dummy_th.dest = usin->sin_port;
	release_sock(sk);
	// 分配一个skb
	buff = sk->prot->wmalloc(sk,MAX_SYN_SIZE,0, GFP_KERNEL);
	if (buff == NULL) 
	{
		return(-ENOMEM);
	}
	sk->inuse = 1;
	// tcp头和选项，告诉对方自己的接收窗口大小1
	buff->len = 24;
	buff->sk = sk;
	buff->free = 0;
	buff->localroute = sk->localroute;
	
	t1 = (struct tcphdr *) buff->data;

	/*
	 *	Put in the IP header and routing stuff. 
	 */
	// 查找路由
	rt=ip_rt_route(sk->daddr, NULL, NULL);
	

	/*
	 *	We need to build the routing stuff from the things saved in skb. 
	 */
	// 构建ip和mac头
	tmp = sk->prot->build_header(buff, sk->saddr, sk->daddr, &dev,
					IPPROTO_TCP, NULL, MAX_SYN_SIZE,sk->ip_tos,sk->ip_ttl);
	if (tmp < 0) 
	{
		sk->prot->wfree(sk, buff->mem_addr, buff->mem_len);
		release_sock(sk);
		return(-ENETUNREACH);
	}

	buff->len += tmp;
	t1 = (struct tcphdr *)((char *)t1 +tmp);

	memcpy(t1,(void *)&(sk->dummy_th), sizeof(*t1));
	// 序列号为初始化的序列号
	t1->seq = ntohl(sk->write_seq++);
	// 下一个数据包中第一个字节的序列号 
	sk->sent_seq = sk->write_seq;
	buff->h.seq = sk->write_seq;
	t1->ack = 0;
	t1->window = 2;
	t1->res1=0;
	t1->res2=0;
	t1->rst = 0;
	t1->urg = 0;
	t1->psh = 0;
	// 是一个syn包
	t1->syn = 1;
	t1->urg_ptr = 0;
	// TCP头包括24个字节，因为还有4个字节的选项
	t1->doff = 6;
	/* use 512 or whatever user asked for */
	
	if(rt!=NULL && (rt->rt_flags&RTF_WINDOW))
		sk->window_clamp=rt->rt_window;
	else
		sk->window_clamp=0;
	// 是否有用户自定义的mss
	if (sk->user_mss)
		sk->mtu = sk->user_mss;
	else if(rt!=NULL && (rt->rt_flags&RTF_MTU))
		sk->mtu = rt->rt_mss;
	else 
	{
#ifdef CONFIG_INET_SNARL
		if ((sk->saddr ^ sk->daddr) & default_mask(sk->saddr))
#else
		if ((sk->saddr ^ sk->daddr) & dev->pa_mask)
#endif
			sk->mtu = 576 - HEADER_SIZE;
		else
			sk->mtu = MAX_WINDOW;
	}
	/*
	 *	but not bigger than device MTU 
	 */

	if(sk->mtu <32)
		sk->mtu = 32;	/* Sanity limit */
		
	sk->mtu = min(sk->mtu, dev->mtu - HEADER_SIZE);
	
	/*
	 *	Put in the TCP options to say MTU. 
	 */
	// 执行tcp头后面的第一个字节
	ptr = (unsigned char *)(t1+1);
	// 选项的类型是2，通知对方TCP报文中数据部分的最大值
	ptr[0] = 2;
	// 选项内容长度是4个字节
	ptr[1] = 4;
	// 组成MSS
	ptr[2] = (sk->mtu) >> 8;
	ptr[3] = (sk->mtu) & 0xff;
	// tcp头的校验和
	tcp_send_check(t1, sk->saddr, sk->daddr,
		  sizeof(struct tcphdr) + 4, sk);

	/*
	 *	This must go first otherwise a really quick response will get reset. 
	 */
	// 设置套接字为syn_send状态
	tcp_set_state(sk,TCP_SYN_SENT);
	// 设置数据包往返时间需要的时间
	sk->rto = TCP_TIMEOUT_INIT;
#if 0 /* we already did this */
	init_timer(&sk->retransmit_timer); 
#endif
	// 设置超时回调
	sk->retransmit_timer.function=&retransmit_timer;
	sk->retransmit_timer.data = (unsigned long)sk;
	// 设置超时时间
	reset_xmit_timer(sk, TIME_WRITE, sk->rto);	/* Timer for repeating the SYN until an answer */
	// 设置syn包的重试次数
	sk->retransmits = TCP_SYN_RETRIES;
	// 发送
	sk->prot->queue_xmit(sk, dev, buff, 0);  
	reset_xmit_timer(sk, TIME_WRITE, sk->rto);
	tcp_statistics.TcpActiveOpens++;
	tcp_statistics.TcpOutSegs++;
  
	release_sock(sk);
	return(0);
}


/* This functions checks to see if the tcp header is actually acceptable. */
extern __inline__ int tcp_sequence(struct sock *sk, struct tcphdr *th, short len,
	     struct options *opt, unsigned long saddr, struct device *dev)
{
	unsigned long next_seq;
	// 数据长度
	next_seq = len - 4*th->doff;
	// fin包有一个EOF字符
	if (th->fin)
		next_seq++;
	/* if we have a zero window, we can't have any data in the packet.. */
	// 本端的窗口大小为0，但是对端传了数据过来
	if (next_seq && !sk->window)
		goto ignore_it;
	// 数据的最后一个字节
	next_seq += th->seq;

	/*
	 * This isn't quite right.  sk->acked_seq could be more recent
	 * than sk->window.  This is however close enough.  We will accept
	 * slightly more packets than we should, but it should not cause
	 * problems unless someone is trying to forge packets.
	 */

	/* have we already seen all of this packet? */
	// 
	if (!after(next_seq+1, sk->acked_seq))
		goto ignore_it;
	/* or does it start beyond the window? */
	// 收到的数据落在可接收的范围内
	if (!before(th->seq, sk->acked_seq + sk->window + 1))
		goto ignore_it;

	/* ok, at least part of this packet would seem interesting.. */
	return 1;

ignore_it:
	if (th->rst)
		return 0;

	/*
	 *	Send a reset if we get something not ours and we are
	 *	unsynchronized. Note: We don't do anything to our end. We
	 *	are just killing the bogus remote connection then we will
	 *	connect again and it will work (with luck).
	 */
  	 
	if (sk->state==TCP_SYN_SENT || sk->state==TCP_SYN_RECV) 
	{
		tcp_reset(sk->saddr,sk->daddr,th,sk->prot,NULL,dev, sk->ip_tos,sk->ip_ttl);
		return 1;
	}

	/* Try to resync things. */
	tcp_send_ack(sk->sent_seq, sk->acked_seq, sk, th, saddr);
	return 0;
}

/*
 *	When we get a reset we do this.
 */

static int tcp_std_reset(struct sock *sk, struct sk_buff *skb)
{
	sk->zapped = 1;
	sk->err = ECONNRESET;
	if (sk->state == TCP_SYN_SENT)
		sk->err = ECONNREFUSED;
	if (sk->state == TCP_CLOSE_WAIT)
		sk->err = EPIPE;
#ifdef TCP_DO_RFC1337		
	/*
	 *	Time wait assassination protection [RFC1337]
	 */
	if(sk->state!=TCP_TIME_WAIT)
	{	
		tcp_set_state(sk,TCP_CLOSE);
		sk->shutdown = SHUTDOWN_MASK;
	}
#else	
	tcp_set_state(sk,TCP_CLOSE);
	sk->shutdown = SHUTDOWN_MASK;
#endif	
	if (!sk->dead) 
		sk->state_change(sk);
	kfree_skb(skb, FREE_READ);
	release_sock(sk);
	return(0);
}

/*
 *	A TCP packet has arrived.
 */
// daddr,saddr是ip头的字段，len为tcp头+数据长度 
int tcp_rcv(struct sk_buff *skb, struct device *dev, struct options *opt,
	unsigned long daddr, unsigned short len,
	unsigned long saddr, int redo, struct inet_protocol * protocol)
{
	struct tcphdr *th;
	struct sock *sk;
	int syn_ok=0;
	
	if (!skb) 
	{
		printk("IMPOSSIBLE 1\n");
		return(0);
	}

	if (!dev) 
	{
		printk("IMPOSSIBLE 2\n");
		return(0);
	}
  
	tcp_statistics.TcpInSegs++;
	// 不是发给本机的，释放读该skb并且增加读缓冲区的大小
	if(skb->pkt_type!=PACKET_HOST)
	{
	  	kfree_skb(skb,FREE_READ);
	  	return(0);
	}
	// tcp头
	th = skb->h.th;

	/*
	 *	Find the socket.
	 */
	// 从tcp的socket哈希链表中找到对应的socket结构
	sk = get_sock(&tcp_prot, th->dest, saddr, th->source, daddr);

	/*
	 *	If this socket has got a reset it's to all intents and purposes 
  	 *	really dead. Count closed sockets as dead.
  	 *
  	 *	Note: BSD appears to have a bug here. A 'closed' TCP in BSD
  	 *	simply drops data. This seems incorrect as a 'closed' TCP doesn't
  	 *	exist so should cause resets as if the port was unreachable.
  	 */
  	// 该socket无效 
	if (sk!=NULL && (sk->zapped || sk->state==TCP_CLOSE))
		sk=NULL;
	// 数据包是新的还是之前被缓存下来的，redo=1表示缓存的
	if (!redo) 
	{	// 检查校验和
		if (tcp_check(th, len, saddr, daddr )) 
		{
			skb->sk = NULL;
			kfree_skb(skb,FREE_READ);
			/*
			 *	We don't release the socket because it was
			 *	never marked in use.
			 */
			return(0);
		}
		th->seq = ntohl(th->seq);

		/* See if we know about the socket. */
		// 找不到sock说明这个数据包无效，发送重置包
		if (sk == NULL) 
		{
			/*
			 *	No such TCB. If th->rst is 0 send a reset (checked in tcp_reset)
			 */
			tcp_reset(daddr, saddr, th, &tcp_prot, opt,dev,skb->ip_hdr->tos,255);
			skb->sk = NULL;
			/*
			 *	Discard frame
			 */
			kfree_skb(skb, FREE_READ);
			return(0);
		}

		skb->len = len;
		skb->acked = 0;
		skb->used = 0;
		skb->free = 0;
		skb->saddr = daddr;
		skb->daddr = saddr;
	
		/* We may need to add it to the backlog here. */
		cli();
		// socket正在被使用，缓存到back_log队列
		if (sk->inuse) 
		{
			skb_queue_tail(&sk->back_log, skb);
			sti();
			return(0);
		}
		sk->inuse = 1;
		sti();
	}
	else
	{
		if (sk==NULL) 
		{
			tcp_reset(daddr, saddr, th, &tcp_prot, opt,dev,skb->ip_hdr->tos,255);
			skb->sk = NULL;
			kfree_skb(skb, FREE_READ);
			return(0);
		}
	}


	if (!sk->prot) 
	{
		printk("IMPOSSIBLE 3\n");
		return(0);
	}


	/*
	 *	Charge the memory to the socket. 
	 */
	// 读缓冲区已满，丢弃数据包
	if (sk->rmem_alloc + skb->mem_len >= sk->rcvbuf) 
	{
		kfree_skb(skb, FREE_READ);
		release_sock(sk);
		return(0);
	}

	skb->sk=sk;
	// 增加读缓冲区已使用的内存的大小
	sk->rmem_alloc += skb->mem_len;

	/*
	 *	This basically follows the flow suggested by RFC793, with the corrections in RFC1122. We
	 *	don't implement precedence and we process URG incorrectly (deliberately so) for BSD bug
	 *	compatibility. We also set up variables more thoroughly [Karn notes in the
	 *	KA9Q code the RFC793 incoming segment rules don't initialise the variables for all paths].
	 */
	// 不是通信数据包，因为连接还没有建立
	if(sk->state!=TCP_ESTABLISHED)		/* Skip this lot for normal flow */
	{
	
		/*
		 *	Now deal with unusual cases.
		 */
		// 是监听socket则可能是一个syn包	
		if(sk->state==TCP_LISTEN)
		{	// 不存在收到ack包的可能，发送重置包
			if(th->ack)	/* These use the socket TOS.. might want to be the received TOS */
				tcp_reset(daddr,saddr,th,sk->prot,opt,dev,sk->ip_tos, sk->ip_ttl);

			/*
			 *	We don't care for RST, and non SYN are absorbed (old segments)
			 *	Broadcast/multicast SYN isn't allowed. Note - bug if you change the
			 *	netmask on a running connection it can go broadcast. Even Sun's have
			 *	this problem so I'm ignoring it 
			 */
			// 不存在这种可能的各种情况，直接丢包			   
			if(th->rst || !th->syn || th->ack || ip_chk_addr(daddr)!=IS_MYADDR)
			{
				kfree_skb(skb, FREE_READ);
				release_sock(sk);
				return 0;
			}
		
			/*	
			 *	Guess we need to make a new socket up 
			 */
			// 是个syn包，建立连接
			tcp_conn_request(sk, skb, daddr, saddr, opt, dev, tcp_init_seq());
		
			/*
			 *	Now we have several options: In theory there is nothing else
			 *	in the frame. KA9Q has an option to send data with the syn,
			 *	BSD accepts data with the syn up to the [to be] advertised window
			 *	and Solaris 2.1 gives you a protocol error. For now we just ignore
			 *	it, that fits the spec precisely and avoids incompatibilities. It
			 *	would be nice in future to drop through and process the data.
			 */
			 
			release_sock(sk);
			return 0;
		}
		// 没有建立连接，又不是listen的socket
		/* retransmitted SYN? */
		// recv状态的时候又收到syn包则丢弃
		// syn_recv状态又收到syn包并且和旧的syn包一样，说明是重传了syn包，直接丢弃
		if (sk->state == TCP_SYN_RECV && th->syn && th->seq+1 == sk->acked_seq)
		{
			kfree_skb(skb, FREE_READ);
			release_sock(sk);
			return 0;
		}
		
		/*
		 *	SYN sent means we have to look for a suitable ack and either reset
		 *	for bad matches or go to connected 
		 */
		// 发送了syn包
		if(sk->state==TCP_SYN_SENT)
		{
			/* Crossed SYN or previous junk segment */
			// 发送了syn包，收到ack包说明可能是建立连接的ack包
			if(th->ack)
			{
				/* We got an ack, but it's not a good ack */
				// ack包不对，发送重置包
				if(!tcp_ack(sk,th,saddr,len))
				{
					/* Reset the ack - its an ack from a 
					   different connection  [ th->rst is checked in tcp_reset()] */
					tcp_statistics.TcpAttemptFails++;
					tcp_reset(daddr, saddr, th,
						sk->prot, opt,dev,sk->ip_tos,sk->ip_ttl);
					kfree_skb(skb, FREE_READ);
					release_sock(sk);
					return(0);
				}
				// 尝试连接但是对端回复了重置包
				if(th->rst)
					return tcp_std_reset(sk,skb);
				// 不合法情况，丢包
				if(!th->syn)
				{
					/* A valid ack from a different connection
					   start. Shouldn't happen but cover it */
					kfree_skb(skb, FREE_READ);
					release_sock(sk);
					return 0;
				}
				/*
				 *	Ok.. it's good. Set up sequence numbers and
				 *	move to established.
				 */
				// 建立连接的回包 
				syn_ok=1;	/* Don't reset this connection for the syn */
				// 期待收到对端下一个的序列号
				sk->acked_seq=th->seq+1;
				sk->fin_seq=th->seq;
				// 发送第三次握手的ack包，进入连接建立状态
				tcp_send_ack(sk->sent_seq,sk->acked_seq,sk,th,sk->daddr);
				tcp_set_state(sk, TCP_ESTABLISHED);
				// 解析tcp选项
				tcp_options(sk,th);
				// 记录对端地址
				sk->dummy_th.dest=th->source;
				// 可以读取但是还没读取的序列号
				sk->copied_seq = sk->acked_seq;
				// 唤醒阻塞在connect函数的进程
				if(!sk->dead)
				{
					sk->state_change(sk);
					sock_wake_async(sk->socket, 0);
				}
				if(sk->max_window==0)
				{
					sk->max_window = 32;
					sk->mss = min(sk->max_window, sk->mtu);
				}
			}
			else
			{
				/* See if SYN's cross. Drop if boring */
				// 对端没有对本端的syn回复，也发了syn包，即同时建立
				if(th->syn && !th->rst)
				{
					/* Crossed SYN's are fine - but talking to
					   yourself is right out... */
					if(sk->saddr==saddr && sk->daddr==daddr &&
						sk->dummy_th.source==th->source &&
						sk->dummy_th.dest==th->dest)
					{
						tcp_statistics.TcpAttemptFails++;
						return tcp_std_reset(sk,skb);
					}
					// 进入syn_recv状态，等待第二次握手的ack
					tcp_set_state(sk,TCP_SYN_RECV);
					
					/*
					 *	FIXME:
					 *	Must send SYN|ACK here
					 */
				}		
				/* Discard junk segment */
				kfree_skb(skb, FREE_READ);
				release_sock(sk);
				return 0;
			}
			/*
			 *	SYN_RECV with data maybe.. drop through
			 */
			goto rfc_step6;
		}

	/*
	 *	BSD has a funny hack with TIME_WAIT and fast reuse of a port. There is
	 *	a more complex suggestion for fixing these reuse issues in RFC1644
	 *	but not yet ready for general use. Also see RFC1379.
	 */
	// 不符合上面的状态
#define BSD_TIME_WAIT
#ifdef BSD_TIME_WAIT
		// 处于2msl状态的socket又收到了一个syn包
		if (sk->state == TCP_TIME_WAIT && th->syn && sk->dead && 
			after(th->seq, sk->acked_seq) && !th->rst)
		{
			long seq=sk->write_seq;
			if(sk->debug)
				printk("Doing a BSD time wait\n");
			tcp_statistics.TcpEstabResets++;	   
			sk->rmem_alloc -= skb->mem_len;
			skb->sk = NULL;
			sk->err=ECONNRESET;
			tcp_set_state(sk, TCP_CLOSE);
			sk->shutdown = SHUTDOWN_MASK;
			release_sock(sk);
			// 是否有一个listen状态的socket，是的话重新建立一个连接
			sk=get_sock(&tcp_prot, th->dest, saddr, th->source, daddr);
			if (sk && sk->state==TCP_LISTEN)
			{
				sk->inuse=1;
				skb->sk = sk;
				sk->rmem_alloc += skb->mem_len;
				tcp_conn_request(sk, skb, daddr, saddr,opt, dev,seq+128000);
				release_sock(sk);
				return 0;
			}
			kfree_skb(skb, FREE_READ);
			return 0;
		}
#endif	
	}

	/*
	 *	We are now in normal data flow (see the step list in the RFC)
	 *	Note most of these are inline now. I'll inline the lot when
	 *	I have time to test it hard and look at what gcc outputs 
	 */
	// 校验包
	if(!tcp_sequence(sk,th,len,opt,saddr,dev))
	{
		kfree_skb(skb, FREE_READ);
		release_sock(sk);
		return 0;
	}
	// 正在通信的时候，对端变成不可写，回复了重置包
	if(th->rst)
		return tcp_std_reset(sk,skb);
	
	/*
	 *	!syn_ok is effectively the state test in RFC793.
	 */
	 
	if(th->syn && !syn_ok)
	{
		tcp_reset(daddr,saddr,th, &tcp_prot, opt, dev, skb->ip_hdr->tos, 255);
		return tcp_std_reset(sk,skb);	
	}

	/*
	 *	Process the ACK
	 */
	 
	// ack包
	if(th->ack && !tcp_ack(sk,th,saddr,len))
	{
		/*
		 *	Our three way handshake failed.
		 */
		 
		if(sk->state==TCP_SYN_RECV)
		{
			tcp_reset(daddr, saddr, th,sk->prot, opt, dev,sk->ip_tos,sk->ip_ttl);
		}
		kfree_skb(skb, FREE_READ);
		release_sock(sk);
		return 0;
	}
	
rfc_step6:		/* I'll clean this up later */

	/*
	 *	Process urgent data
	 */
	 	
	if(tcp_urg(sk, th, saddr, len))
	{
		kfree_skb(skb, FREE_READ);
		release_sock(sk);
		return 0;
	}
	
	
	/*
	 *	Process the encapsulated data
	 */
	
	if(tcp_data(skb,sk, saddr, len))
	{
		kfree_skb(skb, FREE_READ);
		release_sock(sk);
		return 0;
	}

	/*
	 *	And done
	 */	
	
	release_sock(sk);
	return 0;
}

/*
 *	This routine sends a packet with an out of date sequence
 *	number. It assumes the other end will try to ack it.
 */
// 发送一个探测数据包，探测对端是否可写
static void tcp_write_wakeup(struct sock *sk)
{
	struct sk_buff *buff;
	struct tcphdr *t1;
	struct device *dev=NULL;
	int tmp;
	// socket已经被重置
	if (sk->zapped)
		return;	/* After a valid reset we can send no more */

	/*
	 *	Write data can still be transmitted/retransmitted in the
	 *	following states.  If any other state is encountered, return.
	 *	[listen/close will never occur here anyway]
	 */

	if (sk->state != TCP_ESTABLISHED && 
	    sk->state != TCP_CLOSE_WAIT &&
	    sk->state != TCP_FIN_WAIT1 && 
	    sk->state != TCP_LAST_ACK &&
	    sk->state != TCP_CLOSING
	) 
	{
		return;
	}

	buff = sk->prot->wmalloc(sk,MAX_ACK_SIZE,1, GFP_ATOMIC);
	if (buff == NULL) 
		return;

	buff->len = sizeof(struct tcphdr);
	buff->free = 1;
	buff->sk = sk;
	buff->localroute = sk->localroute;

	t1 = (struct tcphdr *) buff->data;

	/* Put in the IP header and routing stuff. */
	tmp = sk->prot->build_header(buff, sk->saddr, sk->daddr, &dev,
				IPPROTO_TCP, sk->opt, MAX_ACK_SIZE,sk->ip_tos,sk->ip_ttl);
	if (tmp < 0) 
	{
		sk->prot->wfree(sk, buff->mem_addr, buff->mem_len);
		return;
	}

	buff->len += tmp;
	t1 = (struct tcphdr *)((char *)t1 +tmp);

	memcpy(t1,(void *) &sk->dummy_th, sizeof(*t1));

	/*
	 *	Use a previous sequence.
	 *	This should cause the other end to send an ack.
	 */
	// 发送一个旧的序列号，期待对端发送ack并且告之当前的接收窗口大小	 
	t1->seq = htonl(sk->sent_seq-1);
	t1->ack = 1; 
	t1->res1= 0;
	t1->res2= 0;
	t1->rst = 0;
	t1->urg = 0;
	t1->psh = 0;
	t1->fin = 0;	/* We are sending a 'previous' sequence, and 0 bytes of data - thus no FIN bit */
	t1->syn = 0;
	// ack为期待对端发送的下一个序列号
	t1->ack_seq = ntohl(sk->acked_seq);
	// 本端的接收窗口大小
	t1->window = ntohs(tcp_select_window(sk));
	t1->doff = sizeof(*t1)/4;
	tcp_send_check(t1, sk->saddr, sk->daddr, sizeof(*t1), sk);
	 /*
	  *	Send it and free it.
   	  *	This will prevent the timer from automatically being restarted.
	  */
	sk->prot->queue_xmit(sk, dev, buff, 1);
	tcp_statistics.TcpOutSegs++;
}

/*
 *	A window probe timeout has occurred.
 */

void tcp_send_probe0(struct sock *sk)
{
	if (sk->zapped)
		return;		/* After a valid reset we can send no more */

	tcp_write_wakeup(sk);

	sk->backoff++;
	sk->rto = min(sk->rto << 1, 120*HZ);
	reset_xmit_timer (sk, TIME_PROBE0, sk->rto);
	sk->retransmits++;
	sk->prot->retransmits ++;
}

/*
 *	Socket option code for TCP. 
 */
  
int tcp_setsockopt(struct sock *sk, int level, int optname, char *optval, int optlen)
{
	int val,err;

	if(level!=SOL_TCP)
		return ip_setsockopt(sk,level,optname,optval,optlen);

  	if (optval == NULL) 
  		return(-EINVAL);

  	err=verify_area(VERIFY_READ, optval, sizeof(int));
  	if(err)
  		return err;
  	
  	val = get_fs_long((unsigned long *)optval);

	switch(optname)
	{
		// 设置mss
		case TCP_MAXSEG:
/*
 * values greater than interface MTU won't take effect.  however at
 * the point when this call is done we typically don't yet know
 * which interface is going to be used
 */
	  		if(val<1||val>MAX_WINDOW)
				return -EINVAL;
			sk->user_mss=val;
			return 0;
		// 设置nagle算法是否开启
		case TCP_NODELAY:
			sk->nonagle=(val==0)?0:1;
			return 0;
		default:
			return(-ENOPROTOOPT);
	}
}

int tcp_getsockopt(struct sock *sk, int level, int optname, char *optval, int *optlen)
{
	int val,err;
	// 获取的是ip层面的选项
	if(level!=SOL_TCP)
		return ip_getsockopt(sk,level,optname,optval,optlen);
			
	switch(optname)
	{	// TCP报文的最大长度，不包括tcp头部，在握手阶段确定，取两端的最小值
		case TCP_MAXSEG:
			val=sk->user_mss;
			break;
		// 是否开启nagle算法
		case TCP_NODELAY:
			val=sk->nonagle;
			break;
		default:
			return(-ENOPROTOOPT);
	}
	err=verify_area(VERIFY_WRITE, optlen, sizeof(int));
	if(err)
  		return err;
  	put_fs_long(sizeof(int),(unsigned long *) optlen);

  	err=verify_area(VERIFY_WRITE, optval, sizeof(int));
  	if(err)
  		return err;
  	put_fs_long(val,(unsigned long *)optval);

  	return(0);
}	


struct proto tcp_prot = {
	sock_wmalloc,
	sock_rmalloc,
	sock_wfree,
	sock_rfree,
	sock_rspace,
	sock_wspace,
	tcp_close,
	tcp_read,
	tcp_write,
	tcp_sendto,
	tcp_recvfrom,
	ip_build_header,
	tcp_connect,
	tcp_accept,
	ip_queue_xmit,
	tcp_retransmit,
	tcp_write_wakeup,
	tcp_read_wakeup,
	tcp_rcv,
	tcp_select,
	tcp_ioctl,
	NULL,
	tcp_shutdown,
	tcp_setsockopt,
	tcp_getsockopt,
	128,
	0,
	{NULL,},
	"TCP",
	0, 0
};
