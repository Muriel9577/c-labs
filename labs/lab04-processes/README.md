# Lab 4. Processes: PID, system calls, fork, exec, wait

Середовище: GitHub Codespaces, Ubuntu 24.04, gcc 13.3.0, strace 6.8, x86-64.

Executable files у репозиторій не додаються (див. `.gitignore`). Окрім файлів з рекомендованої структури, я створив допоміжні програми для експериментів: `new_program.c` і `exec_pid.c` (перевірка PID після exec), `zombie_fixed.c` (zombie з `wait()`), `fork2.c` і `fork3.c` (кількість processes при послідовних `fork()`).

Усі експерименти з процесами, що сплять, я запускав у фоні (`&`), щоб не відкривати другий термінал, і дивився їх через `$!` та `/proc`.

## 1. PID / PPID для декількох instances

`process_info` запущено тричі одночасно:

| Instance | PID | PPID |
|---|---|---|
| Process 1 | 6287 | 1103 |
| Process 2 | 6288 | 1103 |
| Process 3 | 6289 | 1103 |

PPID у всіх 1103 — це PID мого shell (`echo $$` показав 1103). Тому shell є parent для всіх трьох.

Executable `process_info` — це файл на диску (`file`: ELF 64-bit LSB pie executable, x86-64). PID не є властивістю файлу: PID отримує кожен запуск. Один executable створив три різні PID, бо ОС створила три окремі processes з одного program. **Program** — це файл із кодом, а **process** — його запущений екземпляр з власним PID, пам'яттю та станом.

## 2. Аналіз `/proc/PID`

Для запущеного `process_info` (PID 5601):

| Інформація | Результат |
|---|---|
| Process name | process_info |
| PID | 5601 |
| PPID | 1103 (shell) |
| State | S (sleeping) |
| Executable | /workspaces/c-labs/labs/lab04-processes/process_info |

Додатково:

- `cmdline` → `./process_info`;
- `maps` показує області address space процесу: сегменти самого executable (`r--p`, `r-xp`, `rw-p`), `[heap]`, `libc.so.6`, `ld-linux-x86-64.so.2` і `[stack]`;
- коли я перевірив `/proc/PID` після завершення, команда дала `No such file or directory`. Це перший раз сталося випадково (процес спить лише 30 секунд, а я запускав команди з паузою), а в другому запуску я перевірив навмисно: після `kill` каталогу вже не було.

`/proc/PID` не є самим process, це лише вікно ядра для спостереження за його станом. Тому каталог зникає, коли process завершується.

## 3. `write()` через strace

```
$ strace -e write ./hello
write(1, "Hello from process!\n", 20Hello from process!
)   = 20
+++ exited with 0 +++
```

`1` — file descriptor stdout, далі буфер (рядок), `20` — кількість bytes, а після `=` стоїть повернене значення (записано 20 bytes). Повний `strace ./hello` має 37 рядків (завантаження loader, libc, mmap та інше), а наша програма в кінці робить `write` і `exit_group(0)`. Так `printf()` проходить шлях: C library → system call `write` → kernel → stdout.

`output.c` (`printf` і `write` разом):

```
$ strace -e write ./output          (термінал)
write(1, "Output using printf\n", 20) = 20
write(1, "Output using write\n", 19) = 19

$ strace -e write ./output 2>&1 | cat          (stdout у pipe)
write(1, "Output using write\n", 19) = 19
write(1, "Output using printf\n", 20) = 20
```

`printf()` — це функція бібліотеки C з буфером, а `write()` — тонка обгортка над system call. У терміналі stdout буферизується по рядках, тому `printf` записав рядок одразу. У pipe stdout буферизується повністю, тому `printf` записав свій рядок лише при завершенні програми, і порядок змінився. Тому не можна вважати, що кожен `printf()` дає рівно один `write` у той самий момент.

## 4. Результат fork() experiment

```
$ ./fork_basic
Before fork: PID=9561
PARENT: PID=9561, child PID=9562, fork returned=9562
CHILD: PID=9562, PPID=9561, fork returned=0
```

Parent PID = 9561, Child PID = 9562, Child PPID = 9561.

П'ять запусків поспіль показали, що порядок виведення і навіть PPID у child не гарантовані. У трьох із п'яти запусків child вивів `PPID=1`: parent встиг завершитися раніше, і child «усиновив» процес з PID 1. У двох запусках PPID збігся з PID parent. Це підтверджує, що після `fork()` parent і child — окремі processes, і порядок їхнього виконання визначає scheduler.

`strace -f -e trace=process ./fork_basic`:

```
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD ..., child_tidptr=...) = 10872
[pid 10871] exit_group(0)               = ?
[pid 10871] +++ exited with 0 +++
exit_group(0)                           = ?
```

C-функція `fork()` тут реалізована через system call `clone` (а не `fork`), тож назва API і назва kernel-виклику можуть відрізнятися. Значення `10872` — PID child.

## 5. Пояснення значень, що повертає `fork()`

- Перед `fork()` існував 1 process, після успішного `fork()` існує 2 (parent і child).
- Parent отримує PID child (`pid > 0`, у моєму запуску 9562).
- Child отримує `0`.
- Рядок `pid_t pid = fork();` виконується один раз, але повернення з нього відбувається двічі: один раз у parent, один раз у child. Кожен process продовжує з цієї точки зі своїм значенням `pid`, тому `if (pid == 0)` відправляє їх різними шляхами.

```
Process
   │
 fork()
   │
 ┌─┴──────────┐
 ▼            ▼
Parent       Child
pid > 0      pid == 0
```

## 6. Результат `fork_memory.c`

**Прогноз** (записано вже після запуску, тому це не чистий прогноз): я вважав, що child надрукує 300, бо «береться value, а не child». На питання про те, що надрукує parent. Щодо адрес я вважав, що вони будуть різними.

Результат:

```
Before fork: value=100, address=0x593e2580e010
PARENT: PID=10354 value=300 address=0x593e2580e010
CHILD:  PID=10355 value=200 address=0x593e2580e010
```

Другий запуск: усі адреси інші (`0x603e8a04f010`), але знову однакові між parent та child.

Прогноз збігся:

- child надрукував 200 (parent: 300);
- адреси `&value` **однакові** у parent та child.

Після `fork()` parent і child мають окремі address spaces, тому parent, який змінив `value`, не вплинув на child, і навпаки. Однакова virtual address у двох processes не означає ту саму змінювану область пам'яті: кожен process має свою таблицю відображення адрес (це тема virtual memory). Змінюється база між запусками через ASLR, але не між parent та child.

## 7. `wait()` та exit status

```
$ ./wait_basic
PARENT: waiting for child 11686
CHILD: working...
CHILD: finished
PARENT: child finished
PARENT: child exit status = 42
```

Під час `wait()` parent блокується й нічого не виконує, доки child не завершиться. Тому `PARENT: child finished` з'являється після `CHILD: finished`. Число **42** — це не PID, а exit status child (значення з `return 42`). `wait()` повертає в `status` закодоване значення, тому потрібні макроси: `WIFEXITED(status)` перевіряє, що child завершився нормально, а `WEXITSTATUS(status)` витягує сам код.

У `launcher` та `process_runner` exit status теж видно: `false` дає 1, `date` дає 0, а неіснуюча програма (`no_such_program`) завершується зі статусом 1 після `execlp: No such file or directory`.

## 8. Результат exec() experiment

```
$ ./exec_basic
Before exec: PID=12303
Hello from exec!
```

`perror("execlp")` не виконався: успішний `exec()` замінює program image поточного process, тому старий код далі не працює.

Перевірка PID (`exec_pid` запускає `new_program`):

```
Before exec: PID=13156
New program PID=13156
```

| Before exec PID | After exec PID |
|---|---|
| 13156 | 13156 |

Висновок: exec не створює новий process (**new process? NO**), але запускає нову програму (**new program? YES**), а PID лишається той самий (**same PID? YES**).

## 9. Схема fork → exec → wait

Реальний запуск `./fork_exec`:

```
PARENT: PID=14709
PARENT: created child PID=14710
CHILD before exec: PID=14710
Program started by child
PARENT: child completed
```

```
Parent
  │
fork()
  │
┌─┴──────────────┐
│                │
Parent         Child
│                │
│              exec()
│                │
wait()          echo
│                │
│              exit
│                │
◄────────────────┘
│
continue
```

- `fork()` створює child — копію parent.
- `exec()` замінює програму в child (PID той самий, але тепер виконується `echo`).
- `wait()` змушує parent чекати завершення child і забрати його exit status.

## 10. Результат zombie experiment

```
$ ./zombie &
PARENT: PID=17529, child=17531
CHILD: PID=17531, exiting now
$ ps -o pid,ppid,state,cmd --ppid 17529
    PID    PPID S CMD
  17531   17529 Z [zombie] <defunct>
```

Child (17531) уже завершився, але parent (17529) спить і не викликав `wait()`. Стан child — `Z` (zombie, `<defunct>`). Parent при цьому має стан `S` (sleeping).

Версія з `wait()` (`zombie_fixed`):

```
PARENT: PID=18517, child=18518
PARENT: child collected, sleeping
$ ps -o pid,ppid,state,cmd --ppid 18517
    PID    PPID S CMD
```

Дочірнього process у списку немає: parent забрав exit status, і запис про child видалено.

Zombie не виконує instructions і не використовує CPU, а лише лишається записом про завершення process, доки parent не збере status через `wait()`.

## 11. Process tree для двох `fork()`

**Прогноз** Для трьох `fork()` програма `fork3` вивела 8 рядків, тобто **прогноз збігся**. 

Результат (`fork2`, виведено PID та PPID):

```
PID=20617 PPID=1103
PID=20619 PPID=20617
PID=20620 PPID=20617
PID=20621 PPID=20619
```

`printf` виконався 4 рази, тобто **прогноз збігся**.

```
shell (1103)
  └── 20617  (початковий process)
        ├── 20619  (child від першого fork)
        │     └── 20621  (child 20619 від другого fork)
        └── 20620  (child 20617 від другого fork)
```

Перший `fork()` робить 2 processes, і кожен з них виконує другий `fork()`, тому разом їх 4. Для трьох `fork()` програма `fork3` вивела 8 рядків, тобто **прогноз збігся**.

## 12. Аналіз `process_runner`

Програма отримує ім'я програми як аргумент, показує PID parent, робить `fork()`, у child показує PID та PPID і через `execlp` запускає програму. Parent показує PID child, чекає через `waitpid()`, перевіряє, як завершився child (`WIFEXITED`, `WIFSIGNALED`) і продовжує роботу. Помилки `fork`, `exec` та `waitpid` обробляються через `perror`.

Результати:

```
$ ./process_runner date
Parent PID: 21548
Creating child...
Parent: created child with PID 21549
Child PID: 21549
Child PPID: 21548
Executing: date
Sat Nov 14 12:29:49 UTC 2026
Child 21549 completed
Exit status: 0
Parent continues
```

- `./process_runner false` → `Exit status: 1`;
- `./process_runner no_such_program` → child вивів `execlp: No such file or directory` і завершився зі статусом 1, parent це показав;
- `./process_runner` без аргументу → `Usage: ./process_runner <program>`.

Три фрагменти `strace -f -e trace=process ./process_runner date`:

1. **Створення process:**

```
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD ..., child_tidptr=...) = 22197
```

2. **Запуск нового program image:**

```
[pid 22197] execve("/usr/bin/date", ["date"], ...) = 0
```

Перед цим `execlp` перебрав каталоги зі змінної `PATH`, і всі спроби (`/usr/local/bin/date` тощо) повернули `ENOENT (No such file or directory)`, поки не знайшовся `/usr/bin/date`.

3. **Очікування та завершення child:**

```
[pid 22196] wait4(22197, ... <unfinished ...>
[pid 22197] exit_group(0)               = ?
<... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 22197
```

Зіставлення: `fork()` → `clone`, `execlp()` → `execve`, `waitpid()` → `wait4`.

## 13. Відповіді на контрольні питання

1. **Program** — це файл із кодом на диску. **Process** — його запущений екземпляр з PID, власним address space і станом.
2. **PID** — унікальний ідентифікатор process.
3. **PPID** — PID parent process, який створив цей process.
4. Бо кожен запуск створює новий process з власним PID (у мене три запуски `process_info` дали PID 6287, 6288, 6289).
5. `/proc/PID` показує внутрішній стан process (name, state, cmdline, maps, exe). Це лише спосіб спостереження, а не сам process.
6. **System call** — запит програми до kernel на виконання привілейованої операції (наприклад, `write`).
7. Звичайний function call виконується в user space (наприклад, `printf` у бібліотеці), а system call передає керування kernel. `printf` зрештою викликає `write`.
8. `strace` показує system calls, які робить програма (`write`, `clone`, `execve`, `wait4` та інші).
9. `fork()` створює новий process (child) як копію поточного.
10. Після одного успішного `fork()` існує 2 processes.
11. Щоб програма могла розрізнити, хто вона: у child `fork()` повертає 0, а в parent повертає PID child.
12. Parent отримує PID child.
13. Ні. Після `fork()` у parent і child окремі копії змінної: у моєму експерименті parent мав 300, а child 200.
14. Бо virtual address діє лише всередині address space одного process. Однакове число в parent та child вказує на різні області пам'яті кожного з них (тема virtual memory).
15. `exec()` замінює програму, яку виконує поточний process, новою програмою.
16. Ні, process той самий, змінюється лише program image.
17. PID не змінюється (у моєму експерименті 13156 до і після).
18. `wait()` змушує parent чекати завершення child і збирає його exit status, тож child не залишається zombie.
19. **Exit status** — код завершення process (наприклад, 42 у `wait_basic`). Він показує, чи завершилась програма успішно.
20. **Zombie** — завершений child, чий exit status parent ще не зібрав через `wait()`. У `ps` він має стан `Z`.
21. Ні, zombie уже завершив виконання, він лише займає запис у таблиці processes.
22. **Ready** — process готовий виконуватися й чекає CPU. **Running** — виконується на CPU. **Waiting** — чекає події (таймера, I/O) і CPU не потребує. У нашому експерименті `sleep()` дав стан `S (sleeping)`, `%CPU` = 0.0.
23. Після `fork()` parent і child — окремі processes, і їх виконання визначає scheduler. У моїх запусках `PPID` у child то збігався з PID parent, то був 1.
24. Shell робить саме це: `fork()` створює child, у ньому `exec()` запускає введену команду, а parent (shell) через `wait()` чекає її завершення. `launcher` і `process_runner` реалізують цю модель.

## 14. Висновок

У цій лабораторній я побачив різницю між program і process: з одного executable ОС створює кілька processes з різними PID. Через `/proc` та `strace` я спостерігав стан process і system calls: `printf` зрештою призводить до `write`, `fork()` до `clone`, `exec()` до `execve`, `wait()` до `wait4`. Експеримент з `fork_memory` показав, що parent і child мають окремі address spaces, навіть якщо адреси однакові. З `exec()` я побачив, що PID зберігається, але програма змінюється. Експеримент із zombie показав, навіщо parent має викликати `wait()`. Мої прогнози щодо `fork_memory`, двох і трьох `fork()` збіглися, і саме експерименти підтвердив мої прогнози.