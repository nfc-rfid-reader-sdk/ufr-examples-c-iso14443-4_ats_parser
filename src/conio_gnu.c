/*
 * conio_gnu.c
 *
 *  Created on: 31.05.2016.
 *      Author: www.d-logic.net
 */

#if linux || __linux__ || __APPLE__

#include <termios.h>
#include <stdio.h>

static struct termios old, new;

// Initialize new terminal i/o settings:
void _initTermios(int echo)
{
	tcgetattr(0, &old); // grab old terminal i/o settings
	new = old; // make new settings same as old settings
	new.c_lflag &= ~ICANON; // disable buffered i/o
	new.c_lflag &= echo ? ECHO : ~ECHO; // set echo mode
	tcsetattr(0, TCSANOW, &new); // use these new terminal i/o settings now
}

// Restore old terminal i/o settings:
void _resetTermios(void)
{
	tcsetattr(0, TCSANOW, &old);
}

// Read 1 character - echo defines echo mode:
char getch_(int echo)
{
	char ch;
	//initTermios(echo);
	ch = getchar();
	//resetTermios();
	return ch;
}

// Read 1 character without echo:
char _getch(void)
{
	return getchar();//getch_(0);
}

// Read 1 character with echo:
char getche(void)
{
	return getch_(1);
}

// Includes for changemode() and kbhit()
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

/*
int _kbhit(void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}
*/
// _kbhit version 2 {using ioctl}:
///////////////////////////////////////////////////////////////////////////////
#include <sys/ioctl.h>
//#include <termios.h>
int _kbhit(void)
{
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting > 0;
}

/*
Usage:
changemode(1); // or enable_raw_mode();
// ...
if (kbhit()) ...
// ...
changemode(0); // or disable_raw_mode();
tcflush(0, TCIFLUSH); // Clear stdin to prevent characters appearing on prompt
///////////////////////////////////////////////////////////////////////////////
*/

// Usage of the changemode():
// changemode(1);  - before successive calling to _kbhit() and _getchar();
// changemode(0);  - before exit process
void _changeTermiosMode(int dir)
{
  static struct termios oldt, newt;

  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}

// Testing on OSX:
/*
int mygetch( ) {
	struct termios oldt, newt;
	int ch;

	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}
*/

#endif // linux || __linux__ || __APPLE__
