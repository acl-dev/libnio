#include "stdafx.h"
#include "net_msg.h"
#include "net_iostuff.h"

#ifdef SYS_WIN

static int __default_limit = 10240;

int net_open_limit(int limit)
{
	if (limit <= 0) {
		limit = __default_limit;
	}
	return limit;
}

#else
 /*
  * 44BSD compatibility.
  */
#ifndef RLIMIT_NOFILE
#ifdef RLIMIT_OFILE
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
#endif

/* open_limit - set/query file descriptor limit */

int net_open_limit(int limit)
{
	int   rlim_cur = -1;

#ifdef RLIMIT_NOFILE
	struct rlimit rl;

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
#ifdef	ANDROID
		return 10240; /* xxx */
#else
		rlim_cur = getdtablesize();
#endif
		net_msg_warn("%s(%d): getrlimit error: %d, use: %d",
			__FUNCTION__, __LINE__, net_last_error(), rlim_cur);
		return rlim_cur;
	}

	if (rl.rlim_max <= 0) {
		rl.rlim_max = 204800;
	}
	rlim_cur = (int) rl.rlim_cur;

	if (limit > 0) {
		if (limit > (int) rl.rlim_max) {
			rl.rlim_cur = rl.rlim_max;
		} else {
			rl.rlim_cur = limit;
		}
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			net_msg_warn("%s(%d): setrlimit error: %d, limit: %d,"
				" curr: %d", __FUNCTION__, __LINE__,
				net_last_error(), limit, rlim_cur);
			return rlim_cur;
		} else {
			return (int) rl.rlim_cur;
		}
	} else if (rl.rlim_max > rl.rlim_cur) {
		rlim_cur = (int) rl.rlim_cur;
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			net_msg_warn("%s(%d): setrlimit error: %d,"
				" cur: %d, max: %d", __FUNCTION__, __LINE__,
				net_last_error(), (int) rl.rlim_cur,
				(int) rl.rlim_max);
			return rlim_cur;
		}

		return (int) rl.rlim_cur;
	} else {
		return (int) rl.rlim_cur;
	}

#else
	rlim_cur = getdtablesize();
	if (rlim_cur < 0) {
		net_msg_error("%s(%d): getdtablesize(%d) < 0, limit: %d",
			__FUNCTION__, __LINE__, rlim_cur, limit);
	}
	return rlim_cur;
#endif
}

#endif // !SYS_WIN
