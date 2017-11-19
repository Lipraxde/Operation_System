作業系統 作業2 trace xv6-public/proc.c 
===

[Q1.function 之間的關聯，可以用文字或流程圖說明。]()

[Q2.每個 function 做了什麼事。](#各個fountion的介紹)

[Q3.利用 xv6 的 source code 說明其 scheduler 是用哪種排程方法。](#scheduler的排程方法)

[Q4.說明 sv6 在 kernel thread 和 scheduler thread 之間進行 context switch 的機制。](#context-switch如何進行的)

## 數據結構
### proc.h
首先要看懂proc.h裡的cpu、context、proc的struct，這樣看proc.c的時候才會知道程式具體在做什麼。
```c
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
* scheduler，scheduler()的context資訊
* ts，stack的位置
* gdt，存gdt(global descriptor table)
* started，CPU是否啟動
* ncli，pushcli()的深度
* intena，pushcli()前是否有致能中斷？
* \*cpu，拿來存自己？看不出來幹嘛用
* \*proc，目前正在執行的process
```c
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
* chan，跟sleeping()、wakeup()有關，看code像是一個門牌號碼一樣，process可以到某個房子裡休息，wakeup()會叫醒那個房子裡所有的process
* killed，不是0的話process就會被kill
* ofile，開啟的檔案
* cwd，目前所在的目錄
* name，process的名子
```c
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};
```
context儲存context switch會用到的register，xv6跑在使用x86指令集的CPU上。
```c
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
```c
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
初始化ptable用的，會呼叫initlock()來初始化ptable裡的自旋鎖。
### static struct proc\* allocproc(void);
allocproc的用途是在ptable裡面找到一個空位(UNUSED)來存放新的process結構。找到後將那個空的位置的state設置為EMBRYO、配置stack空間給它、將trapret()、forkret()壓入stack，並將nextpid+1。
### void userinit(void);
userinit會產生第一個process。它首先呼叫allocproc()來產生一個process的雛形，並分配pgdir給process，設定name、tf、cwd、state
### int growproc(int n);
growproc用來增加或減少目前process可以存取的memory大小。它會先alloc一塊新的user virtual memory，接著使用switchuvm()更新改變的page table
### int fork(void);
fork用途為複製一個process出來。利用allocproc()獲得process的雛形後，將原本的process的內容複製一份出來。
### void exit(void);
exit用來結束目前的process，關閉所有開啟的檔案、叫醒它的parent process來回收它，在回收前都會是ZOMBIE狀態。
### int wait(void);
wait會對ptable搜尋，找到自己的child process且child process是ZOMBIE態時回收它。
### void scheduler(void)
scheduler是xv6的排程程式，它會不斷地找出處於RUNNABLE狀態的process來執行。
### void sched(void)
sched會將當前的process與scheduler做context switch。
### void yield(void)
yield會對ptable進行上鎖後呼叫sched()。
### void forkret(void)
fork()出來的process第一次被scheduler()抓出來執行時，會從forkret()開始，返回user space。
### void sleep(void \*chan, struct spinlock \*lk)
sleep()會讓process進入SLEEPING狀態，等待wakeup()。
### static void wakeup1(void \*chan)
wakeup1()會在ptable中尋找p->chan==chan且處於SLEEPING狀態的process，並喚醒它(將狀態改成RUNNABLE)。
### void wakeup(void \*chan)
會在呼叫wakeup1()前上鎖，呼叫完後解鎖ptable。
### int kill(int pid)
將某個process kill，會把killed設為1，在trap()的時候process被發現killed==1時，trap()會執行exit()。
### void procdump(void)
將所有process的PID、state、name，如果process正處於SLEEPING狀態，還會輸出它的program conuter。

## scheduler的排程方法
xv6的scheduler()使用的排程方法為Round-robin scheduling。
搜尋處於RUNNABLE的process是使用O(n)線性的搜尋法。
```c
void
scheduler(void)
{
  struct proc *p;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p−>state != RUNNABLE)
        continue;

      // Switch to chosen process. It is the process’s job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p−>state = RUNNING;
      swtch(&cpu−>scheduler, p−>context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p−>state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    
  }
}
```
scheduler()內部是一個無窮迴圈，不斷地sti() → acquire(&ptable.lock) → find process → context switch → release(&ptable.lock)
### sti()
我目前不太清楚scheduler為何在無窮迴圈內要有sti()，xv6有兩個操作pushcli()、popcli()，這兩個會在switchuvm()、acquire()、release()用到，在switchuvm()中的pushcli()、popcli()是成對的，目的是為了避免切換page table時發生中斷，可能會出問題，而acquire()、release()分別使用pushcli()、popcli()，因為如果同時acquire()兩個鎖，就需要有兩次release()，為了避免release()一次就使interrupt enable，使用pushcli()幾次，相對地就要使用popcli()幾次才能使中斷致能回到原本的狀態。
因此對於無窮迴圈內有了個sti()就變得很奇怪，目的是為了使每次進行context switch時可以確保interrupt enable，但pushcli()、popcli()使得cli()、sti()可以成對出現，照理來說每次context switch回scheduler()時interrupt都會處在enable的狀態下，不需要在設定一次阿。
### acquire(&ptable.lock)
取得ptable的鎖(因為其他CPU也同是有自己的schduler()在執行，可是ptable是共用的)，以便進入接下來的critical section。
### find process
```c
for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
  if(p−>state != RUNNABLE)
    continue;
  .
  .
  .
}
```
從這個迴圈可以明顯地看出xv6尋找下一個可執行的process是用Round-robin scheduling，如果process不是屬於RUNNABLE狀態則繼續找下一個，是的話開始進行接下來context switch的動作，當process結束(主動進行context switch或超過時間)，則會從下一個process開始找起。
### context switch
```c
proc = p;
switchuvm(p);
p−>state = RUNNING;
swtch(&cpu−>scheduler, p−>context);
switchkvm();
proc = 0;
```
context switch的開始會將proc設為p(find process所找到的process)並作為目前要執行的process，在context switch結束時將proc設為0。
### release(&ptable.lock)
釋放ptable的鎖，critical section結束。

## context switch如何進行的
這牽扯到scheduler()、trap()、yield()、sched()，還有一些比較底層的function：switchuvm()、switchkvm()、swtch()。
我們要先了解這些function的作用。
### function功能
#### scheduler()解說
我們首先先來看scheduler()這邊
```c
// Switch to chosen process. It is the process’s job
// to release ptable.lock and then reacquire it
// before jumping back to us.
proc = p;
switchuvm(p);
p−>state = RUNNING;
swtch(&cpu−>scheduler, p−>context);
switchkvm();

// Process is done running for now.
// It should have changed its p−>state before coming back.
proc = 0;
```
在更新proc為find process找到的process後，馬上呼叫了switchuvm()，將page table切換為process的，然後在呼叫switch()交換CPU的regsiter。在呼叫switch()後就會切換到process去執行了。
之後要切換成另一個process時，會透過sched()切換回scheduler()。
這邊註解寫到process要將ptable解鎖，然後在跳回scheduler()前要把ptable鎖回去。
#### sched()的解說
```c
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu−>ncli != 1)
    panic("sched locks");
  if(proc−>state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu−>intena;
  swtch(&proc−>context, cpu−>scheduler);
  cpu−>intena = intena;
}
```
sched()會先做一系列的檢查動作：ptable要是鎖上的、ncli深度為1層、必須處於無法中斷的狀態，接著它會備份cpu−>intena後才呼叫swtch()切換至scheduler()(從scheduler切換出來的地方重新開始)，當這個process重新被scheduler()選出來執行時，就會從這邊開始。
#### yield()的解說
```c
void
yield(void)
{
  acquire(&ptable.lock);
  proc−>state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
```
會先將ptable上鎖、process的state改成RUNNABLE(這樣下次scheduler()才會再選到它)後才進入sched()。從sched()出來後會再把ptable解鎖。為什麼要做上鎖解鎖的動作，是因為如果呼叫swtch()沒有上鎖的話，在yield()時會需要先將process的state設成RUNNABLE，還沒有swtch()回scheduler()前，其他CPU可能會將這個process拿去執行，這樣就會造成有兩顆CUP在同一個stack上跑。
#### trap
trap()看起來是會定期幫process呼叫yield()，讓process不會一直占用CUP。另外它還負責檢查process的killed，如果killed!=0的話就會幫這支process呼叫exit()。
trap()應該是被設定成每100ms會經由timer中斷進入的，不過我看不懂trap()是在哪裡被設定的。
#### switchuvm()、switchkvm()、swtch()
switchuvm()：切換至user mode的GDT、載入process的page table
switchkvm()：載入kernel的page table
swtch()：切換CUP的重要regsiter
### context switch切換流程
我們可以把context switch看成兩個動作：
* 從kernel(scheduler)切換到user(process)
* 從user(process)切換回kernel(scheduler)
#### 從kernel切換到user
假設我們有已經建立好的process(不考慮新創的process第一次執行怎麼從forkret、trapret退出並執行program)，而CPU目前正在執行scheduler()，尋找下一個可執行的process。
那我們會依序進行
1. 找到可以執行的process
2. switchuvm()設定CPU的GDT並將process的page table載入(切換成使用process的virtual memory)
3. 呼叫swtch()，切換到process的CUP regsiter
4. 從sched()中的swtch()返回
5. 從sched()返回到yield()
6. yield()將ptable解鎖、返回trap()
7. 從trap()返回之前process執行的地方
注意從第3個步驟到第4個步驟時，CPU已經在不同的stack上執行了，所以並不會從scheduler()裡的swtch()返回。
#### 從user切換回kernel
1. 經過100ms從trap呼叫yield()→步驟2。呼叫exit()這個system call，在裡面進行一些exit的前置處理後呼叫sched()→步驟3
2. 在yield()中將ptable上鎖呼叫sched()
3. sched()檢查完後呼叫swtch()，切換到scheduler()的regsiter
4. 從scheduler()中的swtch()返回
5. switchkvm載入kernel的pagetable
6. 繼續找下一個可以執行的process







幾個問題：
為什麼context switch時先切換了page table可是還是可以修改p->state？這個時候雖然esp還沒有被改變，但是原本的esp指向的位置應該應為page table改變而變到其他地方去了，這樣子呼叫swthc()沒問題嗎？
switchkvm()裡面沒有再載入kernel的GDT，在哪裡解決了這個問題？
為什麼scheduler()裡要有sti()？照道理來說在最後popcli()時就會回到原本的中斷狀態了，有需要特別一直重新enable嗎？
sched()裡面為什麼要備份cpu−>intena？照理來說scheduler()裡面的無窮迴圈會不斷地sti()後acquire，應該是所有的cpu->intena都是處於可中斷的狀態，即使context switch後執行的CPU不同應該也不影響。註解寫的看不懂：
```
Enter scheduler. Must hold only ptable.lock and have changed proc−>state.
Saves and restores intena because intena is a property of this kernel thread, not this CPU.
It should be proc−>intena and proc−>ncli, but that would break in the few places where a lock is held but there’s no process.
```
還沒了解的地方：
system call的機制
interrupt enable/disable的機制
製造新的process的機制(如何載入user program執行)
