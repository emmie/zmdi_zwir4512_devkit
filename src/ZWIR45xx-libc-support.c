#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>



void   __putchar ( char );

caddr_t
_sbrk ( int  incr ) {
	extern char  __heap_start__;
	extern char  __heap_end__;
	static char*  heap_end;
	char* prev_heap_end;

	if ( ! heap_end )
		heap_end = &__heap_start__;
	prev_heap_end = heap_end;

	if ( heap_end + incr >= & __heap_end__ )
		abort ( );

	heap_end += incr;
	return ( caddr_t ) prev_heap_end;
}


int
_write ( int file, char*  ptr, int  len ) {
	int i;
	for ( i = 0; i < len; ++ i )
		__putchar ( * ptr ++ );
	return  len;
}

int
_close ( int  file ) {
	return -1;
}

int
_fstat ( int file, struct stat*  st ) {
	st -> st_mode = S_IFCHR;
	return  0;
}

int
_isatty ( int  file ) {
	return  1;
}

int
_lseek ( int  file, int  ptr,  int  dir ) {
	return  0;
}

int
_read ( int  file, char*  ptr, int  len ) {
	return 0;
}

void
_exit ( int  code ) {
  exit ( code );
}

int
_kill(int pid, int sig){
  return(-1);
}

int
_getpid ( ) {
  return 1;
}
