作業系統 作業2 trace xv6-public/proc.c 
===

## 數據結構
### proc.h
首先要看懂proc.h裡的cpu、context、proc的struct，這樣看proc.c的時候才會知道程式具體在做什麼。
```c=2301
struct cpu {
  uchar apicid; // Local APIC ID
  struct context *scheduler; // swtch() here to enter scheduler
  struct taskstate ts; // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS]; // x86 global descriptor table
  volatile uint started; // Has the CPU started?
  int ncli; // Depth of pushcli nesting.
  int intena; // Were interrupts enabled before pushcli?

  // Cpu−local storage variables; see below
  struct cpu *cpu;
  struct proc *proc; // The currently−running process.
  };
```
struct cpu儲存關於CPU的訊息，包含有：
* APIC ID，APIC(Advanced Programmable Interrupt Controller)的ID
* scheduler，scheduler的context資訊
* ts，stack的位置
* gdt，存gdt(global descriptor table)
* started，CPU是否啟動
* ncli，pushcli的深度
* intena，ushcli前是否有致能中斷？
* \*cpu，拿來存自己？看不出來幹嘛用
* \*proc，目前正在執行的process
```c=2353
struct proc {
  uint sz; // Size of process memory (bytes)
  pde_t* pgdir; // Page table
  char *kstack; // Bottom of kernel stack for this process
  enum procstate state; // Process state
  int pid; // Process ID
  struct proc *parent; // Parent process
  struct trapframe *tf; // Trap frame for current syscall
  struct context *context; // swtch() here to run process
  void *chan; // If non−zero, sleeping on chan
  int killed; // If non−zero, have been killed
  struct file *ofile[NOFILE]; // Open files
  struct inode *cwd; // Current directory
  char name[16]; // Process name (debugging)
};
```
struct proc儲存關於process的訊息，包含有：
* sz，這個process所擁有的memory大小
* pgdir，process的page table
* kstack，這個process的stack最底部的位置
* state，這個process的狀態
* pid，Process ID
* parent，父process
* tf，Trap frame，呼叫system call會用到吧？看不出它怎麼運作
* context，process的context資訊
* chan，跟sleeping、wakeup有關，看code像是一個門牌號碼一樣，process可以到某個房子裡休息，wakeup會叫醒那個房子裡所有的process
* killed，不是0的話process就會被kill
* ofile，開啟的檔案
* cwd，目前所在的目錄
* name，process的名子
```c=2340
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};
```
context儲存context switch會用到的register，xv6跑在使用x86指令集的CPU上。
```c=
extern struct cpu cpus[NCPU];
extern int ncpu;
extern struct cpu *cpu asm("%gs:0"); // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4"); // cpus[cpunum()].proc
enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
```
其他的宣告
* cpus[NCPU]，所有的CPU的struct
* ncpu，可用的CPU的數量
* \*cpu asm("%gs:0")，看不懂這與法
* \*proc asm("%gs:4")，同上，看不懂這與法，看註解是知道所代表的意思，可是不明白為什麼這樣寫
* procstate，struct proc可以有的各個狀態
### proc.c
```c=2409
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;
```
ptable的作用是用來存所有的process的struct，NPROC是最多可以有的process數量，參考xv6/param.h可以得知NPROC=64，所以xv6所擁有的process數量最多為64個。lock則是用來處理同步問題的，因xv6支援多核心，所以需要有鎖來處理同步問題，xv6使用的是自旋鎖。
```c
static struct proc \*initproc;
int nextpid = 1;
```
initproc用來存最初的process
nextpid則是存下一個process創建時所獲得的PID
## 各個fountion的介紹
### void pinit(void);
初始化ptable用的，會呼叫initlock來初始化ptable裡的自旋鎖。
### static struct proc\* allocproc(void);
allocproc的用途是在ptable裡面找到一個空位(UNUSED)來存放新的process結構。找到後將那個空的位置的state設置為EMBRYO、配置stack空間給它、將trapret、forkret壓入stack，並將nextpid+1。
### void userinit(void);
userinit會產生第一個process。它首先呼叫allocproc來產生一個process的雛形，並分配pgdir給process，設定name、tf、cwd、state
### int growproc(int n);
growproc用來增加或減少目前process可以存取的memory大小。它會先alloc一塊新的user virtual memory，接著使用switchuvm更新改變的page table
### int fork(void);
fork用途為複製一個process出來。利用allocproc獲得process的雛形後，將原本的process的內容複製一份出來。
### void exit(void);
exit用來結束目前的process，關閉所有開啟的檔案、叫醒它的parent process來回收它，在回收前都會是ZOMBIE狀態。
### int wait(void);
wait會對ptable搜尋，找到自己的child process且child process是ZOMBIE態時重置它。
### void scheduler(void)
scheduler是xv6的排程程式，它會不斷地找出處於RUNNABLE狀態的process來執行。
### void sched(void)
sched會將當前的process與scheduler做context switch。
### void yield(void)
yield會對ptable進行上鎖後呼叫sched。
### void forkret(void)
fork出來的process第一次被scheduler抓出來執行時，會從forkret開始，返回user space。
### void sleep(void *chan, struct spinlock *lk)
sleep會讓process進入SLEEPING狀態，等待wakeup(chan)。
### static void wakeup1(void \*chan)
wakeup1會在ptable中尋找p->chan==chan且處於SLEEPING狀態的process，並喚醒它(將狀態改成RUNNABLE)。
### void wakeup(void \*chan)
會在呼叫wakeup1前上鎖，呼叫完後解鎖ptable。
### int kill(int pid)
將某個process kill，會把killed設為1，在trap的時候process被發現killed==1時，trap會執行exit。
### void procdump(void)
將所有process的PID、state、name，如果process正處於SLEEPING狀態，還會輸出它的program conuter。



說明process的產生、切換、終止所會呼叫到的function之間的呼叫關係在後面會說明。
## function 之間的關聯，可以用文字或流程圖說明。
## 每個 function 做了什麼事。
## 利用 xv6 的 source code 說明其 scheduler 是用哪種排程方法。
## 說明 sv6 在 kernel thread 和 scheduler thread 之間進行 context switch 的機制。


**getuid**會獲得呼叫它的user的ID。

