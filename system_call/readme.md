作業系統 System Call
===

## sys_call.c:

```c=
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{

    printf("UID = %u\n", getuid());
    
    return 0;
}  
```
這份程式會輸出執行它的user的ID。

## getuid:
**getuid** 宣告在 **unistd.h**
開啟**unistd.h**來看
```c=677
/* Get the real user ID of the calling process.  */
extern __uid_t getuid (void) __THROW;
 ```
**getuid**會獲得呼叫它的user的ID。


Makefile : 產生sys_call程式
