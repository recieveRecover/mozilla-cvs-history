/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
** File:        pripv6.c
** Description: Support for various functions unique to IPv6
*/
#include "primpl.h"
#include <string.h>

#if !defined(_PR_INET6) || defined(_PR_INET6_PROBE)

static PRIOMethods ipv6_to_v4_tcpMethods;
static PRIOMethods ipv6_to_v4_udpMethods;
static PRDescIdentity _pr_ipv6_to_ipv4_id;
extern PRBool IsValidNetAddr(const PRNetAddr *addr);
extern PRIPv6Addr _pr_in6addr_any;
extern PRIPv6Addr _pr_in6addr_loopback;

/*
 * convert an IPv4-mapped IPv6 addr to an IPv4 addr
 */
static void _PR_ConvertToIpv4NetAddr(const PRNetAddr *src_v6addr,
											PRNetAddr *dst_v4addr)
{
const PRUint8 *srcp;

	PR_ASSERT(PR_AF_INET6 == src_v6addr->ipv6.family);

	if (PR_IsNetAddrType(src_v6addr, PR_IpAddrV4Mapped)) {
		srcp = src_v6addr->ipv6.ip.pr_s6_addr;
		memcpy((char *) &dst_v4addr->inet.ip, srcp + 12, 4);
    } else if (PR_IsNetAddrType(src_v6addr, PR_IpAddrAny)) {
        dst_v4addr->inet.ip = htonl(INADDR_ANY);
    } else if (PR_IsNetAddrType(src_v6addr, PR_IpAddrLoopback)) {
        dst_v4addr->inet.ip = htonl(INADDR_LOOPBACK);
    }
	dst_v4addr->inet.family = PR_AF_INET;
	dst_v4addr->inet.port = src_v6addr->ipv6.port;
}

/*
 * convert an IPv4 addr to an IPv4-mapped IPv6 addr
 */
static void _PR_ConvertToIpv6NetAddr(const PRNetAddr *src_v4addr,
                                            PRNetAddr *dst_v6addr)
{
PRUint8 *dstp;

	PR_ASSERT(PR_AF_INET == src_v4addr->inet.family);
	dst_v6addr->ipv6.family = PR_AF_INET6;
	dst_v6addr->ipv6.port = src_v4addr->inet.port;

 	if (htonl(INADDR_ANY) == src_v4addr->inet.ip) {
		dst_v6addr->ipv6.ip = _pr_in6addr_any;
	} else {
		dstp = dst_v6addr->ipv6.ip.pr_s6_addr;
		memset(dstp, 0, 10);
		memset(dstp + 10, 0xff, 2);
		memcpy(dstp + 12,(char *) &src_v4addr->inet.ip, 4);
	}
}

static PRStatus PR_CALLBACK Ipv6ToIpv4SocketBind(PRFileDesc *fd,
								const PRNetAddr *addr)
{
	PRNetAddr tmp_ipv4addr;
	const PRNetAddr *tmp_addrp;
	PRFileDesc *lo = fd->lower;

	if (PR_AF_INET6 != addr->raw.family) {
        PR_SetError(PR_ADDRESS_NOT_SUPPORTED_ERROR, 0);
		return PR_FAILURE;
	}
	if (PR_IsNetAddrType(addr, PR_IpAddrV4Mapped) ||
    			PR_IsNetAddrType(addr, PR_IpAddrAny)) {
		_PR_ConvertToIpv4NetAddr(addr, &tmp_ipv4addr);
		tmp_addrp = &tmp_ipv4addr;
	} else {
        PR_SetError(PR_NETWORK_UNREACHABLE_ERROR, 0);
		return PR_FAILURE;
	}
	return((lo->methods->bind)(lo,tmp_addrp));
}

static PRStatus PR_CALLBACK Ipv6ToIpv4SocketConnect(
    PRFileDesc *fd, const PRNetAddr *addr, PRIntervalTime timeout)
{
	PRNetAddr tmp_ipv4addr;
	const PRNetAddr *tmp_addrp;

	if (PR_AF_INET6 != addr->raw.family) {
        PR_SetError(PR_ADDRESS_NOT_SUPPORTED_ERROR, 0);
		return PR_FAILURE;
	}
	if (PR_IsNetAddrType(addr, PR_IpAddrV4Mapped) ||
			PR_IsNetAddrType(addr, PR_IpAddrLoopback)) {
		_PR_ConvertToIpv4NetAddr(addr, &tmp_ipv4addr);
		tmp_addrp = &tmp_ipv4addr;
	} else {
        PR_SetError(PR_NETWORK_UNREACHABLE_ERROR, 0);
		return PR_FAILURE;
	}
	return (fd->lower->methods->connect)(fd->lower, tmp_addrp, timeout);
}

static PRInt32 PR_CALLBACK Ipv6ToIpv4SocketSendTo(
    PRFileDesc *fd, const void *buf, PRInt32 amount,
    PRIntn flags, const PRNetAddr *addr, PRIntervalTime timeout)
{
	PRNetAddr tmp_ipv4addr;
	const PRNetAddr *tmp_addrp;

	if (PR_AF_INET6 != addr->raw.family) {
        PR_SetError(PR_ADDRESS_NOT_SUPPORTED_ERROR, 0);
		return PR_FAILURE;
	}
	if (PR_IsNetAddrType(addr, PR_IpAddrV4Mapped) ||
			PR_IsNetAddrType(addr, PR_IpAddrLoopback)) {
		_PR_ConvertToIpv4NetAddr(addr, &tmp_ipv4addr);
		tmp_addrp = &tmp_ipv4addr;
	} else {
        PR_SetError(PR_NETWORK_UNREACHABLE_ERROR, 0);
		return PR_FAILURE;
	}
    return (fd->lower->methods->sendto)(
        fd->lower, buf, amount, flags, tmp_addrp, timeout);
}

static PRFileDesc* PR_CALLBACK Ipv6ToIpv4SocketAccept (
    PRFileDesc *fd, PRNetAddr *addr, PRIntervalTime timeout)
{
    PRStatus rv;
    PRFileDesc *newfd;
    PRFileDesc *newstack;
	PRNetAddr tmp_ipv4addr;
    PRNetAddr *addrlower = NULL;

    PR_ASSERT(fd != NULL);
    PR_ASSERT(fd->lower != NULL);

    newstack = PR_NEW(PRFileDesc);
    if (NULL == newstack)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }
    *newstack = *fd;  /* make a copy of the accepting layer */

    if (addr)
        addrlower = &tmp_ipv4addr;
    newfd = (fd->lower->methods->accept)(fd->lower, addrlower, timeout);
    if (NULL == newfd)
    {
        PR_DELETE(newstack);
        return NULL;
    }
    if (addr)
        _PR_ConvertToIpv6NetAddr(&tmp_ipv4addr, addr);

    rv = PR_PushIOLayer(newfd, PR_TOP_IO_LAYER, newstack);
    PR_ASSERT(PR_SUCCESS == rv);
    return newfd;  /* that's it */
}

static PRInt32 PR_CALLBACK Ipv6ToIpv4SocketAcceptRead(PRFileDesc *sd,
			PRFileDesc **nd, PRNetAddr **ipv6_raddr, void *buf, PRInt32 amount,
							PRIntervalTime timeout)
{
    PRInt32 nbytes;
    PRStatus rv;
	PRNetAddr tmp_ipv4addr;
    PRFileDesc *newstack;

    PR_ASSERT(sd != NULL);
    PR_ASSERT(sd->lower != NULL);

    newstack = PR_NEW(PRFileDesc);
    if (NULL == newstack)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return -1;
    }
    *newstack = *sd;  /* make a copy of the accepting layer */

    nbytes = sd->lower->methods->acceptread(
        sd->lower, nd, ipv6_raddr, buf, amount, timeout);
    if (-1 == nbytes)
    {
        PR_DELETE(newstack);
        return nbytes;
    }
	tmp_ipv4addr = **ipv6_raddr;	/* copy */
	_PR_ConvertToIpv6NetAddr(&tmp_ipv4addr, *ipv6_raddr);

    /* this PR_PushIOLayer call cannot fail */
    rv = PR_PushIOLayer(*nd, PR_TOP_IO_LAYER, newstack);
    PR_ASSERT(PR_SUCCESS == rv);
    return nbytes;
}

static PRStatus PR_CALLBACK Ipv6ToIpv4SocketGetName(PRFileDesc *fd,
										PRNetAddr *ipv6addr)
{
	PRStatus result;
	PRNetAddr tmp_ipv4addr;

	result = (fd->lower->methods->getsockname)(fd->lower, &tmp_ipv4addr);
	if (PR_SUCCESS == result) {
		_PR_ConvertToIpv6NetAddr(&tmp_ipv4addr, ipv6addr);
		PR_ASSERT(IsValidNetAddr(ipv6addr) == PR_TRUE);
	}
	return result;
}

static PRStatus PR_CALLBACK Ipv6ToIpv4SocketGetPeerName(PRFileDesc *fd,
										PRNetAddr *ipv6addr)
{
	PRStatus result;
	PRNetAddr tmp_ipv4addr;

	result = (fd->lower->methods->getsockname)(fd->lower, &tmp_ipv4addr);
	if (PR_SUCCESS == result) {
		_PR_ConvertToIpv6NetAddr(&tmp_ipv4addr, ipv6addr);
		PR_ASSERT(IsValidNetAddr(ipv6addr) == PR_TRUE);
	}
	return result;
}

static PRInt32 PR_CALLBACK Ipv6ToIpv4SocketRecvFrom(PRFileDesc *fd, void *buf,
			PRInt32 amount, PRIntn flags, PRNetAddr *ipv6addr,
				PRIntervalTime timeout)
{
	PRNetAddr tmp_ipv4addr;
	PRInt32 result;

    result = (fd->lower->methods->recvfrom)(
        fd->lower, buf, amount, flags, &tmp_ipv4addr, timeout);
	if (-1 != result) {
		_PR_ConvertToIpv6NetAddr(&tmp_ipv4addr, ipv6addr);
		PR_ASSERT(IsValidNetAddr(ipv6addr) == PR_TRUE);
	}
	return result;
}

#if defined(_PR_INET6_PROBE)
PRBool _pr_ipv6_is_present;
PR_EXTERN(PRBool) _pr_test_ipv6_socket();
#if defined(_PR_HAVE_GETIPNODEBYNAME)
void *_pr_getipnodebyname_fp;
void *_pr_getipnodebyaddr_fp;
void *_pr_freehostent_fp;
#endif
#endif

PRStatus _pr_init_ipv6()
{
    const PRIOMethods *stubMethods;

#if defined(_PR_INET6_PROBE)

#if !defined(_PR_INET6) && defined(_PR_HAVE_GETIPNODEBYNAME)
	PRLibrary *lib;	
	_pr_getipnodebyname_fp = PR_FindSymbolAndLibrary("getipnodebyname", &lib);
	if (NULL != _pr_getipnodebyname_fp) {
		_pr_freehostent_fp = PR_FindSymbol(lib, "freehostent");
		if (NULL != _pr_freehostent_fp) {
			_pr_getipnodebyaddr_fp = PR_FindSymbol(lib, "getipnodebyaddr");
			if (NULL != _pr_getipnodebyaddr_fp)
				_pr_ipv6_is_present = PR_TRUE;
			else
				_pr_ipv6_is_present = PR_FALSE;
		} else
			_pr_ipv6_is_present = PR_FALSE;
		(void)PR_UnloadLibrary(lib);
	} else
		_pr_ipv6_is_present = PR_FALSE;
	if (PR_TRUE == _pr_ipv6_is_present)
#endif
	
	_pr_ipv6_is_present = _pr_test_ipv6_socket();
	if (PR_TRUE == _pr_ipv6_is_present)
			return PR_SUCCESS;
#endif

    _pr_ipv6_to_ipv4_id = PR_GetUniqueIdentity("Ipv6_to_Ipv4 layer");
    PR_ASSERT(PR_INVALID_IO_LAYER != _pr_ipv6_to_ipv4_id);

	stubMethods = PR_GetDefaultIOMethods();

	ipv6_to_v4_tcpMethods = *stubMethods;  /* first get the entire batch */
	/* then override the ones we care about */
	ipv6_to_v4_tcpMethods.connect = Ipv6ToIpv4SocketConnect;
	ipv6_to_v4_tcpMethods.bind = Ipv6ToIpv4SocketBind;
	ipv6_to_v4_tcpMethods.accept = Ipv6ToIpv4SocketAccept;
	ipv6_to_v4_tcpMethods.acceptread = Ipv6ToIpv4SocketAcceptRead;
	ipv6_to_v4_tcpMethods.getsockname = Ipv6ToIpv4SocketGetName;
	ipv6_to_v4_tcpMethods.getpeername = Ipv6ToIpv4SocketGetPeerName;
/*
	ipv6_to_v4_tcpMethods.getsocketoption = Ipv6ToIpv4GetSocketOption;
	ipv6_to_v4_tcpMethods.setsocketoption = Ipv6ToIpv4SetSocketOption;
*/
	ipv6_to_v4_udpMethods = *stubMethods;  /* first get the entire batch */
	/* then override the ones we care about */
	ipv6_to_v4_udpMethods.connect = Ipv6ToIpv4SocketConnect;
	ipv6_to_v4_udpMethods.bind = Ipv6ToIpv4SocketBind;
	ipv6_to_v4_udpMethods.sendto = Ipv6ToIpv4SocketSendTo;
	ipv6_to_v4_udpMethods.recvfrom = Ipv6ToIpv4SocketRecvFrom;
	ipv6_to_v4_tcpMethods.getsockname = Ipv6ToIpv4SocketGetName;
	ipv6_to_v4_tcpMethods.getpeername = Ipv6ToIpv4SocketGetPeerName;
/*
	ipv6_to_v4_tcpMethods.getsocketoption = Ipv6ToIpv4GetSocketOption;
	ipv6_to_v4_tcpMethods.setsocketoption = Ipv6ToIpv4SetSocketOption;
*/
	return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) _pr_push_ipv6toipv4_layer(PRFileDesc *fd)
{
	PRFileDesc *ipv6_fd = NULL;

	/*
	 * For platforms with no support for IPv6 
	 * create layered socket for IPv4-mapped IPv6 addresses
	 */
	if (fd->methods->file_type == PR_DESC_SOCKET_TCP)
		ipv6_fd = PR_CreateIOLayerStub(_pr_ipv6_to_ipv4_id,
									&ipv6_to_v4_tcpMethods);
	else
		ipv6_fd = PR_CreateIOLayerStub(_pr_ipv6_to_ipv4_id,
									&ipv6_to_v4_udpMethods);
	if (NULL == ipv6_fd) {
		goto errorExit;
	} 
	ipv6_fd->secret = NULL;

	if (PR_PushIOLayer(fd, PR_TOP_IO_LAYER, ipv6_fd) == PR_FAILURE) {
		goto errorExit;
	}

	return PR_SUCCESS;
errorExit:

	if (ipv6_fd)
		ipv6_fd->dtor(ipv6_fd);
	return PR_FAILURE;
}

#endif /* !defined(_PR_INET6) || defined(_PR_INET6_PROBE) */
