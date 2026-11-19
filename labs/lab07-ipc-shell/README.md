# Lab 7. Pipes, redirection, signals і mini-shell

У цій лабораторній я об'єднав те, що вивчав раніше (processes, file descriptors, I/O), і побудував невеликий shell. Основна ідея така: shell не є частиною ядра. Це звичайна user-space програма, яка складається з системних викликів `fork()`, `execvp()`, `waitpid()`, `pipe()`, `dup2()`, `open()`, `close()` і signals.

Середовище: GitHub Codespaces (Ubuntu 24.04), gcc 13.3.0, strace 6.8. Програми компілювалися з `-Wall -Wextra`.

## Файли

| Файл | Призначення |
|---|---|
| `pipe_basic.c` | pipe всередині одного процесу |
| `pipe_processes.c` | pipe між parent і child |
| `pipe_no_close.c` | експеримент із незакритим write end (дві збірки: `-DCLOSE_UNUSED` і без нього) |
| `dup2_output.c` | перенаправлення stdout у файл |
| `redirect_exec.c` | `fork` + `dup2` + `execvp("ls -l")` у файл |
| `pipeline.c` | `ls -l \| wc -l` вручну |
| `signal_int.c` | handler для SIGINT через `sigaction()` |
| `sigaction_kill.c` | додатковий файл: спроба встановити handler для SIGKILL |
| `signal_parent_child.c` | SIGTERM від parent до child (дві збірки: default і `-DCUSTOM_HANDLER`) |
| `mini_shell.c` | фінальний mini-shell |

## Прогнози

Чесно про те, коли я їх записав.

- **Прогноз 1** -
- **Прогноз 2** У варіанті А виводиться технічна причина (що саме «впало», сигнал), а у варіанті Б власний статус.
- **Прогноз 3** (SIGKILL для `signal_int`): я вважав, що ядро просто так перехопити сигнал не дає, а що зробить shell, не знав.

Прогнози 2 і 3  Їхній зміст збігається з результатами експериментів, результати експериментів описані нижче у відповідних розділах.

---

## 1. Read end і write end pipe

`pipe(pipefd)` створює в ядрі буфер і повертає два file descriptors:

- `pipefd[0]` — **read end**, з нього читають через `read()`;
- `pipefd[1]` — **write end**, у нього пишуть через `write()`.

Дані рухаються в один бік: усе, що записано у write end, можна прочитати з read end в тому ж порядку (FIFO). Pipe не є regular file: на диску його немає, це об'єкт у пам'яті ядра. Читати і писати в нього можна через `read()`/`write()`, бо ці виклики працюють з будь-яким file descriptor, а не лише з файлами.

Результат `pipe_basic`:

```
Read end: 3
Write end: 4
Received: Hello through pipe!
```

Дескриптори 0, 1, 2 уже зайняті stdin/stdout/stderr, тому pipe отримав найменші вільні номери 3 і 4. Тут один процес сам пише і сам читає, це лише перевірка механізму.

## 2. Descriptor map для `pipe_processes.c`

Одразу після `pipe()` (у strace це `pipe2([3, 4], 0)`):

```
Parent: 0,1,2 → terminal   3 → pipe READ   4 → pipe WRITE
```

Після `fork()` child отримує **копії** тих самих дескрипторів, які вказують на ті самі pipe ends:

```
Parent                      Child
3 → pipe READ               3 → pipe READ
4 → pipe WRITE              4 → pipe WRITE
```

Потім кожен закриває те, що йому не потрібно:

```
Parent: close(3)  (read end)      Parent залишає: 4 → pipe WRITE
Child:  close(4)  (write end)     Child залишає:  3 → pipe READ

Parent ──write(4)──▶ [ PIPE ] ──read(3)──▶ Child
```

Вивід програми: `Child received: Message from parent`.

## 3. Експеримент з незакритим descriptor (`pipe_no_close`)

Child читає pipe в циклі до EOF. Я зібрав два варіанти:

- `pipe_no_close_ok` — child закриває свій `pipefd[1]`;
- `pipe_no_close_hang` — child **не** закриває свій `pipefd[1]`.

Результати:

```
--- З правильним close ---
Child received: Message from parent
Child: EOF reached
exit: 0

--- БЕЗ close у child ---
Child received: Message from parent
```

У другому варіанті програма не завершилася. Рядка `Child: EOF reached` немає, цикл `read()` завис. `ps` показав два живих процеси:

```
PID   PPID STAT WCHAN    CMD
4493  1507 S    do_wait   ./pipe_no_close_hang      (parent)
4495  4493 S    pipe_read ./pipe_no_close_hang      (child)
```

- parent сидить у `waitpid()` (`do_wait`) і чекає child;
- child сидить у `read()` на pipe (`pipe_read`) і чекає даних.

Вони чекають один на одного: parent чекає завершення child, а child чекає EOF, якого не буде.

Під час експерименту `pkill -x pipe_no_close_hang` не спрацював: `pkill` повідомив, що патерн довший за 15 символів (ім'я процесу в ядрі обрізається), тому збігів немає, і обидва процеси лишилися живими. Тому я завершив їх через `pkill -f pipe_no_close_hang`. Результат: ⟦вивід `ps` після `pkill -f`⟧.

## 4. Чому pipe порожній ≠ EOF

Порожній pipe ще не означає EOF. `read()` повертає 0 (EOF) тільки коли виконуються **дві** умови одночасно:

```
pipe порожній  +  усі write ends закриті
```

Якщо хоч один write end десь відкритий, ядро вважає, що писач ще може щось записати, і `read()` блокується. У моєму експерименті таким «писачем» був сам child: після `fork()` він мав власну копію `pipefd[1]` і сам її не закрив. Parent свою копію закрив, але цього недостатньо. Тому правило: **кожен процес закриває ті кінці pipe, які він не використовує**.

## 5. Короткий strace pipe experiment

Команда: `strace -f -e trace=pipe,pipe2,clone,fork,vfork,read,write,close,wait4 ./pipe_processes`. Ключові рядки (parent має PID 3976, child 3977):

```
pipe2([3, 4], 0) = 0                                  ← створення pipe
clone(... SIGCHLD) = 3977                             ← fork
[pid 3976] close(3) = 0                               ← parent закриває read end
[pid 3976] write(4, "Message from parent\n", 20) = 20 ← parent пише
[pid 3977] close(4) = 0                               ← child закриває write end
[pid 3976] close(4) = 0                               ← parent закриває write end
[pid 3977] read(3, "Message from parent\n", 127) = 20 ← child читає
[pid 3976] wait4(3977, ...                            ← parent чекає
[pid 3977] write(1, "Child received: ...", 36) = 36   ← child виводить на екран
[pid 3977] close(3) = 0
[pid 3977] +++ exited with 0 +++
<... wait4 resumed>NULL, 0, NULL) = 3977              ← parent дочекався
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=3977, ...} ---
```

Усі потрібні операції видно: `pipe2` → `clone` → `close` → `write` → `read` → `close` → `wait4`. Також видно, що ядро відправило parent signal SIGCHLD, коли child завершився.

## 6. Що робить `dup2()`

`dup2(fd, STDOUT_FILENO)` робить так, що дескриптор 1 починає вказувати на той самий об'єкт, що й `fd`. Попередню ціль дескриптора 1 (термінал) вона закриває.

```
До:                    Після dup2(fd, 1):         Після close(fd):
fd 1 → terminal        fd 1 ─┐                    fd 1 → redirected.txt
fd 3 → redirected.txt  fd 3 ─┴→ redirected.txt
```

`printf()` при цьому не змінився: він, як і раніше, пише у stdout, тобто в дескриптор 1. Змінилося лише те, на який ресурс вказує дескриптор 1. Результат `dup2_output`: на екран нічого не вивелося, а `cat redirected.txt` показав `This goes to the file`.

## 7. Descriptor map до і після redirection

Для `redirect_exec.c` (child, який запускає `ls -l`):

```
Після fork (child):      Після open():           Після dup2(fd, 1) + close(fd):
0 → terminal             0 → terminal            0 → terminal
1 → terminal             1 → terminal            1 → ls-output.txt
2 → terminal             2 → terminal            2 → terminal
                         3 → ls-output.txt
```

Після `execvp("ls")` процес `ls` отримує ці ж дескриптори і просто пише у fd 1. Він не знає, що це файл. Результат: програма завершилась зі статусом 0, а `ls-output.txt` містить `total 132` і список файлів.

Цікава деталь: у самому `ls-output.txt` файл `ls-output.txt` записаний з розміром **0**. Це тому, що child створив і відкрив цей файл **до** запуску `ls`, тож `ls` побачив його в директорії, але даних у ньому ще не було. Пізніше (у виводі `ls -l` з mini-shell) той самий файл уже має розмір 942 байти.

## 8. Пояснення `ls -l | wc -l` (`pipeline.c`)

Parent створює pipe, потім двох children:

- **Child 1:** `dup2(pipefd[1], STDOUT_FILENO)`, закриває `pipefd[0]` і `pipefd[1]`, виконує `ls -l`;
- **Child 2:** `dup2(pipefd[0], STDIN_FILENO)`, закриває `pipefd[0]` і `pipefd[1]`, виконує `wc -l`;
- **Parent:** закриває обидва кінці і робить `waitpid()` для обох.

Результат: `./pipeline` вивів `17`, і `ls -l | wc -l` у звичайному shell теж дав `17`. Результати збігаються.

## 9. Descriptor map pipeline

```
Перед dup2():
Child 1: 0 → terminal   1 → terminal   3 → pipe READ   4 → pipe WRITE
Child 2: 0 → terminal   1 → terminal   3 → pipe READ   4 → pipe WRITE

Після dup2() і close():
Child 1: 0 → terminal   1 → pipe WRITE
Child 2: 0 → pipe READ  1 → terminal

Після exec():
ls (stdout) ──▶ [ PIPE ] ──▶ wc (stdin)
```

Parent закриває свої копії обох кінців. Якби parent залишив write end відкритим, `wc` ніколи не отримав би EOF і чекав би вічно.

## 10. Результати signal experiments

**SIGINT (`signal_int`).** Я надсилав сигнал командою `kill -INT` (а не клавішею Ctrl+C), щоб керувати з одного терміналу:

```
SIGINT received
SIGINT received
--- процес ще живий? ---
7792 S ./signal_int
```

Handler спрацював двічі, і процес лишився живим (стан S, він чекає в `pause()`).

**SIGKILL.** `kill -KILL` завершив процес відразу, handler не викликався:

```
[2]+ Killed ./signal_int
exit status after SIGKILL: 137
```

Статус 137 = 128 + 9 (номер SIGKILL).

**Спроба встановити handler (`sigaction_kill`):**

```
sigaction(SIGKILL) failed: Invalid argument
sigaction(SIGTERM) succeeded
```

**SIGTERM між parent і child (`signal_parent_child`):**

```
--- default SIGTERM ---
Child: PID=8848 waiting for signal
Parent: sending SIGTERM to child 8848
Parent: child killed by signal 15

--- власний handler ---
Child: PID=8909 waiting for signal
Parent: sending SIGTERM to child 8909
Child: SIGTERM received, exiting gracefully
Parent: child exited with status 0
```

У варіанті з default поведінкою child убитий сигналом (`WIFSIGNALED`, сигнал 15). У варіанті з handler child сам завершується зі статусом 0 (`WIFEXITED`). Тож `kill(pid, SIGTERM)` не означає «знищити процес», а лише «надіслати SIGTERM»; що буде далі, залежить від того, як процес обробляє цей signal.

## 11. SIGINT, SIGTERM, SIGKILL, SIGCHLD

| Signal | Звідки | Можна перехопити? | Default |
|---|---|---|---|
| SIGINT (2) | Ctrl+C у терміналі | так | завершити процес |
| SIGTERM (15) | `kill pid`, ввічливе прохання завершитись | так | завершити процес |
| SIGKILL (9) | `kill -KILL`, гарантоване знищення | **ні** | завершити процес |
| SIGCHLD (17) | ядро → parent, коли child завершився | так | ігнорується |

SIGKILL не можна перехопити, заблокувати чи ігнорувати: інакше процес, що завис, неможливо було б зупинити. Це підтвердив експеримент: `sigaction(SIGKILL)` повернув `Invalid argument`. SIGCHLD я бачив у strace (`--- SIGCHLD ... CLD_EXITED ---`), коли child завершився.

## 12. Архітектура mini-shell

```
START → вивести "myshell> " → fgets() → tokenize (strtok) → розбір
   │
   ├── exit        → вихід з головного циклу (без fork)
   ├── cd [dir]    → chdir() у самому shell (без fork)
   ├── cmd [> file]→ fork → child: (SIG_DFL, open+dup2+close) → execvp ; parent: waitpid
   └── cmd1 | cmd2 → pipe → fork ×2 → dup2 → close обох кінців → execvp ; parent: close обох + waitpid ×2
   └→ наступний prompt
```

Основні функції:

- `tokenize()` — ділить рядок на токени за пробілами/табами (`strtok`). `|` і `>` мають бути відокремлені пробілами;
- `parse_command()` — збирає `argv` і розпізнає `> file`;
- `child_restore_signals()` — у child повертає SIGINT у `SIG_DFL`;
- `child_redirect_output()` — `open(O_WRONLY|O_CREAT|O_TRUNC)` → `dup2` → `close`;
- `wait_for()` — `waitpid()` із повтором при `EINTR`;
- `run_simple()` — одна команда, можливо з `>`;
- `run_pipeline()` — дві команди через pipe (перенаправлення `>` дозволено лише для правої команди);
- `main()` — ігнорує SIGINT, цикл «prompt → читання → розбір → виконання».

Додатково: обмежені довжина рядка (1024) і кількість аргументів (64); надто довгий рядок відкидається з повідомленням; `system()` не використовується; більше однієї `|` shell відхиляє з повідомленням.

## 13. Чому `cd` це built-in

`cd` змінює **поточну директорію процесу**, який його виконує. Якби shell запускав `cd` через `fork` + `exec` (або через `fork` + `chdir`), директорія змінилась би лише в child, який одразу завершиться, а parent-shell залишився б у старій директорії. Тому `cd` виконується самим shell через `chdir()` без `fork()`. Те саме стосується `exit`: завершити shell може лише сам shell.

Перевірка (див. тест нижче): `pwd` до `cd /tmp` показав `/workspaces/c-labs/labs/lab07-ipc-shell`, а після нього `/tmp`.

## 14. Результати redirection test

Тест через `printf '...' | ./mini_shell` (stdin не є терміналом, тому prompt `myshell> ` виводиться без переносів рядків і склеюється з виводом, це лише вигляд, а не помилка):

```
myshell> echo test > test.txt
myshell> cat test.txt
test
```

Другий тест:

```
myshell> echo hello > output.txt
myshell> cat output.txt
hello
```

Файл створюється, `echo` нічого не пише на екран (його stdout перенаправлено), `cat` показує вміст.

## 15. Результати pipeline test

```
myshell> ls -l | wc -l
11
myshell> cat output.txt | grep hello
hello
```

Число 11 — кількість рядків виводу `ls -l` у поточній директорії shell (на той момент це `/tmp` після `cd /tmp`). `cat output.txt | grep hello` також працює.

Перевірки помилок (shell не завершується і знову показує prompt):

```
myshell> no_such_command           → execvp: No such file or directory
myshell> cd /no_such_dir           → cd: No such file or directory
myshell> ls |                      → myshell: syntax error near '|'
myshell> | wc -l                   → myshell: syntax error near '|'
myshell> echo a | cat | cat        → myshell: only one pipe is supported
```

Після кожної помилки наступні команди (`echo shell-still-alive`, `echo after-cd-error`) виконались нормально.

Результати інших пунктів послідовності з завдання: `pwd` → `/workspaces/c-labs/labs/lab07-ipc-shell`, `ls -l` показав список файлів, `echo Hello Systems` → `Hello Systems`, `cd /tmp` + `pwd` → `/tmp`, `exit` завершив shell.

## 16. Результат Ctrl+C test

Інтерактивний запуск:

```
myshell> sleep 30
^Cmyshell> sleep 30
^Cmyshell> echo still-running
still-running
myshell> exit
```

`sleep 30` двічі перервався одразу після Ctrl+C (не через 30 секунд), shell не завершився і одразу показав новий prompt, а `echo still-running` виконався. Причина: shell встановлює `SIGINT → SIG_IGN`, а перед `execvp()` кожен child повертає `SIGINT → SIG_DFL`. Це потрібно, бо `SIG_IGN` (на відміну від власного handler) **зберігається після exec**; без скидання `sleep` теж ігнорував би Ctrl+C. Термінал надсилає SIGINT усій foreground-групі: shell його ігнорує, а `sleep` помирає.

## 17. Короткий аналіз strace mini-shell

Команда: `printf 'echo hello > output.txt\nls | wc -l\nexit\n' | strace -f -e trace=process,pipe,pipe2,dup2,openat,close,read,write ./mini_shell`. Виводив лише ключові рядки.

**`echo hello > output.txt`** (shell має PID 14503):

```
clone(...SIGCHLD) = 14504                               ← fork child
wait4(14504, <unfinished ...>                           ← parent чекає
[14504] openat(AT_FDCWD, "output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 3
[14504] dup2(3, 1) = 1                                  ← stdout → файл
[14504] execve("/usr/bin/echo", ["echo", "hello"], ...) = 0
wait4 resumed ... WEXITSTATUS(s) == 0                   ← child завершився
```

Тут точно видна послідовність з коду: `fork` → `open` (дескриптор 3) → `dup2(3, 1)` → `execve`.

Перед успішним `execve("/usr/bin/echo", ...)` було багато невдалих `execve` з `ENOENT` для директорій із `PATH` (`/vscode/...`, `/home/codespace/.local/bin/echo` тощо). Так працює `execvp()`: вона по черзі пробує кожну директорію з `PATH`, поки не знайде виконуваний файл.

**`ls | wc -l`:**

```
pipe2([3, 4], 0) = 0                                    ← створення pipe
clone(...) = 14505                                      ← child 1 (ls)
clone(...) = 14506                                      ← child 2 (wc)
[14505] dup2(4, 1) = 1                                  ← stdout ls → pipe write
[14506] dup2(3, 0) = 0                                  ← stdin wc ← pipe read
[14505] execve("/usr/bin/ls", ["ls"], ...) = 0
[14506] execve("/usr/bin/wc", ["wc", "-l"], ...) = 0
[14505] write(1, "dup2_output\ndup2_output.c\n...", 390) = 390   ← ls пише у pipe
[14506] read(0, "dup2_output\ndup2_output.c\n...", 16320) = 390   ← wc читає з pipe
wait4(14505 ...) ... wait4(14506 ...)                    ← parent чекає обох
```

Видно, що `ls` записав у pipe 390 байтів, а `wc` прочитав ті самі 390 байтів зі свого stdin. Parent викликає `wait4` для обох children по черзі. (`close` у моєму фільтрі `grep` не відображався, тому закриття зайвих кінців у цьому уривку не видно, але те, що `wc` завершився й shell не завис, показує, що зайві кінці закриті правильно.)

## 18. Відповіді на контрольні питання

1. **Що таке IPC?** Inter-Process Communication — механізми обміну даними й сигналами між процесами (pipes, signals, shared memory, sockets тощо).
2. **Що створює `pipe()`?** Однонаправлений буфер у ядрі та два file descriptors: для читання і для запису.
3. **Що означає `pipefd[0]`?** Read end pipe, з нього читають.
4. **Що означає `pipefd[1]`?** Write end pipe, у нього пишуть.
5. **Чому pipe можна використовувати через `read()`/`write()`?** Бо pipe представлений звичайним file descriptor, а `read()` і `write()` працюють з будь-яким дескриптором, незалежно від того, що за ним стоїть (файл, термінал, pipe).
6. **Які descriptors отримує child після `fork()`?** Копії всіх відкритих дескрипторів parent (0, 1, 2 та обидва кінці pipe), які вказують на ті самі об'єкти ядра.
7. **Чому потрібно закривати unused pipe ends?** Щоб не було зайвих копій write end (інакше не настане EOF), щоб не витрачати дескриптори і щоб кожен процес мав лише ту роль (читач або писач), яка йому потрібна.
8. **Коли `read()` із pipe повертає EOF?** Коли pipe порожній і всі write ends (у всіх процесах) закриті; тоді `read()` повертає 0.
9. **Що може статися, якщо parent залишить write end відкритим?** Reader ніколи не отримає EOF і зависне в `read()`; у моєму експерименті так зависла пара parent/child.
10. **Що робить `dup2()`?** `dup2(old, new)` робить так, що дескриптор `new` вказує на той самий об'єкт, що й `old` (попередній `new` спочатку закривається).
11. **Як реалізується `>`?** У child перед `exec`: `open(file, O_WRONLY|O_CREAT|O_TRUNC)` → `dup2(fd, STDOUT_FILENO)` → `close(fd)` → `execvp()`.
12. **Чому program не повинна знати, що її stdout redirected?** Бо вона пише у fd 1, а що за ним (термінал, файл, pipe), вирішує shell до `exec`. Це робить програми універсальними.
13. **Як побудувати `program1 | program2`?** `pipe()` → два `fork()` → у першій `dup2(pipefd[1], 1)`, у другій `dup2(pipefd[0], 0)` → обидві закривають обидва кінці pipe → `execvp()`; parent закриває обидва кінці й чекає обох.
14. **Який descriptor першої program підключити до pipe?** stdout (fd 1), до write end.
15. **Який descriptor другої program підключити до pipe?** stdin (fd 0), до read end.
16. **Що таке signal?** Асинхронне повідомлення ядра процесу про подію (Ctrl+C, завершення child, `kill`), яке процес обробляє за замовчуванням або через handler.
17. **Чим signal відрізняється від pipe?** Signal несе лише номер події і приходить асинхронно, а pipe передає потік байтів, який процес читає, коли йому потрібно.
18. **Що робить `kill()`?** Надсилає вказаний signal процесу; саме по собі не гарантує завершення, усе залежить від обробки signal.
19. **Чим SIGTERM відрізняється від SIGKILL?** SIGTERM можна перехопити (дозволяє акуратно завершитись), SIGKILL не можна перехопити чи ігнорувати, процес знищується ядром.
20. **Чому не можна catch SIGKILL?** Щоб в адміністратора і ядра завжди був гарантований спосіб зупинити процес. Моя перевірка: `sigaction(SIGKILL)` повернув `Invalid argument`.
21. **Що таке SIGINT?** Signal переривання, який термінал надсилає foreground-процесам при Ctrl+C; за замовчуванням завершує процес.
22. **Чому `printf()` не є хорошим вибором для signal handler?** Бо `printf` використовує внутрішні буфери і блокування; якщо signal перервав саму програму посеред `printf` або `malloc`, повторний виклик із handler може зіпсувати стан чи зависнути.
23. **Що таке async-signal-safe function?** Функція, яку безпечно викликати з signal handler, навіть якщо вона перервала іншу функцію (наприклад, `write()`, `_exit()`).
24. **Чому shell є user-space program, а не частиною kernel?** Бо shell не робить нічого, чого не можна зробити системними викликами: читає введення, викликає `fork`/`exec`/`wait`/`pipe`/`dup2`. Це звичайна програма, яку можна замінити (bash, sh, мій mini-shell).
25. **Що робить `fork()` у shell?** Створює копію процесу shell (child), у якій потім можна налаштувати дескриптори і запустити іншу програму, не знищуючи сам shell.
26. **Що робить `exec()`?** Замінює код і дані поточного процесу новою програмою; PID і відкриті дескриптори зберігаються, після успішного `exec` попередній код не продовжується.
27. **Для чого потрібен `waitpid()`?** Щоб shell дочекався завершення child (перед наступним prompt) і прибрав його запис, не залишивши zombie.
28. **Чому `cd` має бути built-in?** Бо поточна директорія є властивістю процесу; зміна в child не вплине на shell (див. розділ 13).
29. **Чому `exit` має виконуватися самим shell?** Бо завершується процес, який виконує команду; якби `exit` запускався в child, завершився б лише child, а shell лишився б працювати.
30. **Чому `system()` не підходить?** Бо `system()` запускає ще один shell (`/bin/sh -c`) і приховує саме ті механізми (`fork`, `exec`, `pipe`, `dup2`), які треба вивчити.
31. **Що відбувається з file descriptors після `exec()`?** Вони лишаються відкритими (крім позначених `O_CLOEXEC`), тому redirection, зроблений перед `exec`, працює в новій програмі. Signal handlers скидаються на default, а `SIG_IGN` зберігається (тому mini-shell повертає SIGINT у `SIG_DFL` у child).
32. **Як `fork()`, `pipe()` і `dup2()` разом створюють pipeline?** `pipe()` дає два кінці, `fork()` створює дві дочірні програми, що успадковують кінці, `dup2()` підключає кінці до stdin/stdout, `exec()` запускає програми, які просто читають із stdin і пишуть у stdout.
33. **Чому pipeline processes можуть виконуватися concurrently?** Бо це різні процеси, створені заздалегідь; ядро планує їх незалежно (на кількох ядрах справді паралельно). Pipe має буфер: письменник блокується, коли буфер повний, а читач, коли порожній.
34. **Що відбувається, якщо reader pipe завершився, а writer продовжує писати?** Для запису в pipe без жодного reader ядро надсилає writer signal SIGPIPE (за замовчуванням завершує його); якщо SIGPIPE ігнорується, `write()` повертає помилку `EPIPE`.
35. **Які ресурси повинен закривати parent після створення pipeline?** Обидва кінці pipe (`pipefd[0]` і `pipefd[1]`); після цього він викликає `waitpid()` для обох children.

## 19. Висновок

У цій лабораторній я зібрав усе вивчене раніше в одну програму. Я побачив, що pipe — це об'єкт ядра, що з'єднує два процеси через пару дескрипторів. Експеримент із незакритим write end показав, що EOF настає лише коли закриті всі писачі: у моєму випадку parent і child зависли, бо child тримав свою копію `pipefd[1]`. Redirection через `dup2()` не змінює саму програму, а лише те, на що вказують fd 0/1/2, і ця зміна переживає `exec()`. Pipeline — це `pipe()` + два `fork()` + `dup2()` + `exec()` + закриття зайвих кінців.

Signals показали, що `kill()` лише надсилає signal, а результат залежить від його обробки: default SIGTERM убиває child, власний handler дозволяє завершитись із статусом 0, а SIGKILL перехопити не можна взагалі. Для mini-shell довелося врахувати, що `SIG_IGN` зберігається після `exec()`, тому child перед запуском команди повертає SIGINT у default.

У strace я побачив ті ж кроки, що закладені в код mini-shell: `clone` → `openat` → `dup2` → `execve` для redirection і `pipe2` → два `clone` → `dup2(4,1)`/`dup2(3,0)` → `execve` → `wait4` для pipeline. Це підтвердило, що shell не робить нічого магічного: він складається зі звичайних системних викликів. Mini-shell підтримує зовнішні команди, `cd`, `exit`, `>`, один pipe та Ctrl+C, а при помилках (`execvp`, `chdir`, синтаксис) продовжує працювати.