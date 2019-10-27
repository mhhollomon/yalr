#if not defined(YALR_YASSERT_HPP)
#define YALR_YASSERT_HPP

#include <cstdio>
#include <cstdlib>

[[noreturn]] inline void _yassert(const char* expression, 
	const char *mesg, const char* file, int line) noexcept {

	fprintf(stderr, "Assertion '%s' failed, file '%s' line '%d': %s\n", 
            expression, file, line, mesg);
	abort();
}

[[noreturn]] inline void _yfail( const char *mesg, 
                                const char* file, int line) noexcept {

	fprintf(stderr, "Internal error, file '%s' line '%d': %s\n", file, line, mesg);
	abort();
}
 
#define yassert(EXPR, MESG) ((EXPR) ? (void)0 : _yassert(#EXPR, MESG, __FILE__, __LINE__))
#define yfail(MESG) (_yfail(MESG, __FILE__, __LINE__))

#endif
