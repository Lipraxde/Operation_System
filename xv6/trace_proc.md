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
* *cpu，拿來存自己？看不出來幹嘛用
* *proc，目前正在執行的process

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
* kstack，這個process的stack
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
* *cpu asm("%gs:0")，看不懂這與法
* *proc asm("%gs:4")，同上，看不懂這與法，看註解是知道所代表的意思，可是不明白為什麼這樣寫
* procstate，struct proc可以有的各個狀態

### proc.c




```sequence
Alice->Bob: Hello Bob, how are you?
Note right of Bob: Bob thinks
Bob-->Alice: I am good thanks!
Note left of Alice: Alice responds
Alice->Bob: Where have you been?
```

```c=
// 初始化用的
// 產生、消滅 process 用的
// 控制 process 狀態相關的
// process 排程相關的
// 其他類
```

## function 之間的關聯，可以用文字或流程圖說明。
## 每個 function 做了什麼事。
## 利用 xv6 的 source code 說明其 scheduler 是用哪種排程方法。
## 說明 sv6 在 kernel thread 和 scheduler thread 之間進行 context switch 的機制。


**getuid**會獲得呼叫它的user的ID。

