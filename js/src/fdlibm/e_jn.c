/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* @(#)e_jn.c 1.4 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
 * __ieee754_jn(n, x), __ieee754_yn(n, x)
 * floating point Bessel's function of the 1st and 2nd kind
 * of order n
 *          
 * Special cases:
 *	y0(0)=y1(0)=yn(n,0) = -inf with division by zero signal;
 *	y0(-ve)=y1(-ve)=yn(n,-ve) are NaN with invalid signal.
 * Note 2. About jn(n,x), yn(n,x)
 *	For n=0, j0(x) is called,
 *	for n=1, j1(x) is called,
 *	for n<x, forward recursion us used starting
 *	from values of j0(x) and j1(x).
 *	for n>x, a continued fraction approximation to
 *	j(n,x)/j(n-1,x) is evaluated and then backward
 *	recursion is used starting from a supposed value
 *	for j(n,x). The resulting value of j(0,x) is
 *	compared with the actual value to correct the
 *	supposed value of j(n,x).
 *
 *	yn(n,x) is similar in all respects, except
 *	that forward recursion is used for all
 *	values of n>1.
 *	
 */

#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
invsqrtpi=  5.64189583547756279280e-01, /* 0x3FE20DD7, 0x50429B6D */
two   =  2.00000000000000000000e+00, /* 0x40000000, 0x00000000 */
one   =  1.00000000000000000000e+00; /* 0x3FF00000, 0x00000000 */

static double zero  =  0.00000000000000000000e+00;

#ifdef __STDC__
	double __ieee754_jn(int n, double x)
#else
	double __ieee754_jn(n,x)
	int n; double x;
#endif
{
        fd_twoints u;
	int i,hx,ix,lx, sgn;
	double a, b, temp, di;
	double z, w;

    /* J(-n,x) = (-1)^n * J(n, x), J(n, -x) = (-1)^n * J(n, x)
     * Thus, J(-n,x) = J(n,-x)
     */
        u.d = x;
	hx = __HI(u);
	ix = 0x7fffffff&hx;
	lx = __LO(u);
    /* if J(n,NaN) is NaN */
	if((ix|((unsigned)(lx|-lx))>>31)>0x7ff00000) return x+x;
	if(n<0){		
		n = -n;
		x = -x;
		hx ^= 0x80000000;
	}
	if(n==0) return(__ieee754_j0(x));
	if(n==1) return(__ieee754_j1(x));
	sgn = (n&1)&(hx>>31);	/* even n -- 0, odd n -- sign(x) */
	x = fd_fabs(x);
	if((ix|lx)==0||ix>=0x7ff00000) 	/* if x is 0 or inf */
	    b = zero;
	else if((double)n<=x) {   
		/* Safe to use J(n+1,x)=2n/x *J(n,x)-J(n-1,x) */
	    if(ix>=0x52D00000) { /* x > 2**302 */
    /* (x >> n**2) 
     *	    Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
     *	    Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
     *	    Let s=sin(x), c=cos(x), 
     *		xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
     *
     *		   n	sin(xn)*sqt2	cos(xn)*sqt2
     *		----------------------------------
     *		   0	 s-c		 c+s
     *		   1	-s-c 		-c+s
     *		   2	-s+c		-c-s
     *		   3	 s+c		 c-s
     */
		switch(n&3) {
		    case 0: temp =  fd_cos(x)+fd_sin(x); break;
		    case 1: temp = -fd_cos(x)+fd_sin(x); break;
		    case 2: temp = -fd_cos(x)-fd_sin(x); break;
		    case 3: temp =  fd_cos(x)-fd_sin(x); break;
		}
		b = invsqrtpi*temp/fd_sqrt(x);
	    } else {	
	        a = __ieee754_j0(x);
	        b = __ieee754_j1(x);
	        for(i=1;i<n;i++){
		    temp = b;
		    b = b*((double)(i+i)/x) - a; /* avoid underflow */
		    a = temp;
	        }
	    }
	} else {
	    if(ix<0x3e100000) {	/* x < 2**-29 */
    /* x is tiny, return the first Taylor expansion of J(n,x) 
     * J(n,x) = 1/n!*(x/2)^n  - ...
     */
		if(n>33)	/* underflow */
		    b = zero;
		else {
		    temp = x*0.5; b = temp;
		    for (a=one,i=2;i<=n;i++) {
			a *= (double)i;		/* a = n! */
			b *= temp;		/* b = (x/2)^n */
		    }
		    b = b/a;
		}
	    } else {
		/* use backward recurrence */
		/* 			x      x^2      x^2       
		 *  J(n,x)/J(n-1,x) =  ----   ------   ------   .....
		 *			2n  - 2(n+1) - 2(n+2)
		 *
		 * 			1      1        1       
		 *  (for large x)   =  ----  ------   ------   .....
		 *			2n   2(n+1)   2(n+2)
		 *			-- - ------ - ------ - 
		 *			 x     x         x
		 *
		 * Let w = 2n/x and h=2/x, then the above quotient
		 * is equal to the continued fraction:
		 *		    1
		 *	= -----------------------
		 *		       1
		 *	   w - -----------------
		 *			  1
		 * 	        w+h - ---------
		 *		       w+2h - ...
		 *
		 * To determine how many terms needed, let
		 * Q(0) = w, Q(1) = w(w+h) - 1,
		 * Q(k) = (w+k*h)*Q(k-1) - Q(k-2),
		 * When Q(k) > 1e4	good for single 
		 * When Q(k) > 1e9	good for double 
		 * When Q(k) > 1e17	good for quadruple 
		 */
	    /* determine k */
		double t,v;
		double q0,q1,h,tmp; int k,m;
		w  = (n+n)/(double)x; h = 2.0/(double)x;
		q0 = w;  z = w+h; q1 = w*z - 1.0; k=1;
		while(q1<1.0e9) {
			k += 1; z += h;
			tmp = z*q1 - q0;
			q0 = q1;
			q1 = tmp;
		}
		m = n+n;
		for(t=zero, i = 2*(n+k); i>=m; i -= 2) t = one/(i/x-t);
		a = t;
		b = one;
		/*  estimate log((2/x)^n*n!) = n*log(2/x)+n*ln(n)
		 *  Hence, if n*(log(2n/x)) > ...
		 *  single 8.8722839355e+01
		 *  double 7.09782712893383973096e+02
		 *  long double 1.1356523406294143949491931077970765006170e+04
		 *  then recurrent value may overflow and the result is 
		 *  likely underflow to zero
		 */
		tmp = n;
		v = two/x;
		tmp = tmp*__ieee754_log(fd_fabs(v*tmp));
		if(tmp<7.09782712893383973096e+02) {
	    	    for(i=n-1,di=(double)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
	     	    }
		} else {
	    	    for(i=n-1,di=(double)(i+i);i>0;i--){
		        temp = b;
			b *= di;
			b  = b/x - a;
		        a = temp;
			di -= two;
		    /* scale b to avoid spurious overflow */
			if(b>1e100) {
			    a /= b;
			    t /= b;
			    b  = one;
			}
	     	    }
		}
	    	b = (t*__ieee754_j0(x)/b);
	    }
	}
	if(sgn==1) return -b; else return b;
}

#ifdef __STDC__
	double __ieee754_yn(int n, double x) 
#else
	double __ieee754_yn(n,x) 
	int n; double x;
#endif
{
        fd_twoints u;
	int i,hx,ix,lx;
	int sign;
	double a, b, temp;

        u.d = x;
	hx = __HI(u);
	ix = 0x7fffffff&hx;
	lx = __LO(u);
    /* if Y(n,NaN) is NaN */
	if((ix|((unsigned)(lx|-lx))>>31)>0x7ff00000) return x+x;
	if((ix|lx)==0) return -one/zero;
	if(hx<0) return zero/zero;
	sign = 1;
	if(n<0){
		n = -n;
		sign = 1 - ((n&1)<<1);
	}
	if(n==0) return(__ieee754_y0(x));
	if(n==1) return(sign*__ieee754_y1(x));
	if(ix==0x7ff00000) return zero;
	if(ix>=0x52D00000) { /* x > 2**302 */
    /* (x >> n**2) 
     *	    Jn(x) = cos(x-(2n+1)*pi/4)*sqrt(2/x*pi)
     *	    Yn(x) = sin(x-(2n+1)*pi/4)*sqrt(2/x*pi)
     *	    Let s=sin(x), c=cos(x), 
     *		xn=x-(2n+1)*pi/4, sqt2 = sqrt(2),then
     *
     *		   n	sin(xn)*sqt2	cos(xn)*sqt2
     *		----------------------------------
     *		   0	 s-c		 c+s
     *		   1	-s-c 		-c+s
     *		   2	-s+c		-c-s
     *		   3	 s+c		 c-s
     */
		switch(n&3) {
		    case 0: temp =  fd_sin(x)-fd_cos(x); break;
		    case 1: temp = -fd_sin(x)-fd_cos(x); break;
		    case 2: temp = -fd_sin(x)+fd_cos(x); break;
		    case 3: temp =  fd_sin(x)+fd_cos(x); break;
		}
		b = invsqrtpi*temp/fd_sqrt(x);
	} else {
	    a = __ieee754_y0(x);
	    b = __ieee754_y1(x);
	/* quit if b is -inf */
            u.d = b;
	    for(i=1;i<n&&(__HI(u) != 0xfff00000);i++){ 
		temp = b;
		b = ((double)(i+i)/x)*b - a;
		a = temp;
	    }
	}
	if(sign>0) return b; else return -b;
}
