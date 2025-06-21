/* creates and reads a fifo to communicate with spotifyd's event shell
 */

#ifndef _FIFO_H_
#define _FIFO_H_

#include <vdr/thread.h>
#include <vdr/tools.h>
#include <sys/unistd.h>

class cFifo : public cThread
{
  public:
    cFifo(void);
    virtual ~cFifo();
    //bool isOpen() { return spotfifo > (FILE*) NULL; }
    //bool isOpen() { return spotfifo > 0; }

  protected:

    int spotfifo;
    void Stop();
    void Action();
};
#endif // _FIFO_H_
