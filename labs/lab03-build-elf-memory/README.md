# Lab 3. Побудова програми, ELF та організація пам'яті

Середовище: GitHub Codespaces, Ubuntu 24.04, gcc 13.3.0, binutils 2.42, glibc 2.39, x86-64.

Усі етапи можна відтворити з вихідних `.c` файлів. Файли `*.i`, `*.s`, `*.o` та executable у репозиторій не додаються (див. `.gitignore`).

## 1. Етапи build pipeline

```
program.c → (gcc -E) → program.i → (gcc -S) → program.s → (gcc -c) → program.o → (linker) → program
```

| Файл | Етап | Що містить | Чи може CPU виконати безпосередньо? |
|---|---|---|---|
| `.c` | source | текст програми мовою C | ні |
| `.i` | після preprocessor | усе ще C, але `#include` розгорнуто (829 рядків проти 16) | ні |
| `.s` | після compiler | текст на Assembly x86-64 (Intel syntax) | ні |
| `.o` | після assembler | machine code в ELF relocatable object, адреси ще не остаточні | ні |
| executable | після linker | ELF executable (PIE) з machine code, секціями та залежністю від `libc.so.6` | так, після завантаження OS |

Результати `file` для `memory_map`:

```
memory_map.c: C source, ASCII text
memory_map.i: C source, ASCII text
memory_map.s: assembler source, ASCII text
memory_map.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
memory_map:   ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, ... not stripped
```

Окремі спостереження:

- `program.i` має 829 рядків, а `program.c` лише 16. Решту додав preprocessor з `stdio.h` та вкладених заголовків. `#include <stdio.h>` у `program.i` уже немає, замість нього стоїть вміст заголовків.
- `program.s` має мітки `add:` (рядок 21) і `main:` (рядок 47).
- `cat program.o` показує нечитабельний вигляд, бо це бінарний файл. Тому я дивився `head -c 64 program.o | cat -v`. Початок файлу: `^?ELF^B^A^A...`, тобто магічне число `7f 45 4c 46`.
- `objdump -d -M intel program.o` для `add`:

```
0:  f3 0f 1e fa    endbr64
4:  55             push rbp
5:  48 89 e5       mov  rbp,rsp
8:  89 7d fc       mov  DWORD PTR [rbp-0x4],edi
b:  89 75 f8       mov  DWORD PTR [rbp-0x8],esi
e:  8b 55 fc       mov  edx,DWORD PTR [rbp-0x4]
11: 8b 45 f8       mov  eax,DWORD PTR [rbp-0x8]
14: 01 d0          add  eax,edx
16: 5d             pop  rbp
17: c3             ret
```

- Symbols у `program.o` (`nm program.o`):

```
0000000000000000 T add
0000000000000000 B global_empty
0000000000000000 D global_value
0000000000000018 T main
                 U printf
```

`T` означає, що функція визначена в `.text`. `D` позначає ініціалізовану глобальну змінну, `B` позначає неініціалізовану. `U` означає undefined: реалізація `printf` не в цьому файлі.

## 2. Linker error та її виправлення

`gcc -c main.c -o main.o` пройшло успішно. Потім:

```
$ gcc main.o -o app
/usr/bin/ld: main.o: in function `main':
main.c:(.text+0x17): undefined reference to `calculate'
collect2: error: ld returned 1 exit status
```

**Чому compilation пройшла, а linking ні.** Для компіляції `main.c` достатньо оголошення `int calculate(int a, int b);`. Компілятор знає тип функції й залишає в `main.o` виклик з невизначеним symbol. `nm main.o` показує `U calculate`. Linker має знайти реалізацію в одному з об'єктних файлів чи бібліотек, і не знайшов.

**Виправлення.** Я створив `calculate.c`, скомпілював його в `calculate.o` (`nm calculate.o` → `T calculate`) і зібрав обидва файли разом:

```
$ gcc main.o calculate.o -o app
$ ./app
30
```

`main.o` потребує `calculate` (`U`), а `calculate.o` визначає його (`T`). Linker їх з'єднав.

## 3. Аналіз ELF (`readelf -h program`)

| ELF property | Значення |
|---|---|
| Class | ELF64 |
| Data | 2's complement, little endian |
| Type | DYN (Position-Independent Executable file) |
| Machine | Advanced Micro Devices X86-64 |
| Entry point | 0x1060 |

ELF64 означає 64-бітний формат файлу. Machine показує архітектуру x86-64 (AMD64). Type DYN означає PIE: програма може завантажуватися за довільною адресою. Entry point 0x1060 збігається з початком `.text` (дивіться розділ 4).

## 4. Секції `.text`, `.rodata`, `.data`, `.bss`

Дані з `readelf -S -W program` та `nm program`:

| Section | Адреса | Розмір | Що містить (у нашій програмі) |
|---|---|---|---|
| `.text` | 0x1060 | 0x142 | machine code: `add` (0x1149), `main` (0x1161) та службовий код старту |
| `.rodata` | 0x2000 | 0x10 | константи, зокрема рядок формату `"Result: %d\n"` |
| `.data` | 0x4000 | 0x14 | ініціалізовані змінні: `global_value = 10` (0x4010, `D`) |
| `.bss` | 0x4014 | 0x0c | неініціалізовані змінні: `global_empty` (0x4018, `B`) |

`global_value = 10` лежить у `.data`, бо має початкове значення, яке треба зберігати у файлі. `global_empty` лежить у `.bss`, бо початкового значення немає й воно за замовчуванням нуль. Тип секції `.bss` у `readelf` — `NOBITS`, тобто у файлі місця вона не займає.

Перевірка на `sections.c` (`nm sections`):

```
0000000000004010 D initialized
0000000000002010 R message
0000000000004014 d static_initialized
0000000000004020 b static_uninitialized
000000000000401c B uninitialized
```

- `initialized` та `static_initialized` у `.data` (0x4010 та 0x4014). У `objdump -s -j .data` на цих адресах видно `7b000000` (123) і `c8010000` (456 = 0x1c8). Порядок байтів little-endian.
- `uninitialized` та `static_uninitialized` у `.bss` (0x401c та 0x4020).
- `message` у `.rodata` (0x2010, `R`). У `objdump -s -j .rodata` видно рядок `Computer Systems`.
- Малі літери (`d`, `b`) у `nm` означають `static`, тобто локальний для файлу symbol.

## 5. Експеримент з великою `.bss`

**Прогноз:** ні, розмір файлу не збільшиться приблизно на 400 000 bytes. Моє пояснення тоді було таке: комп'ютер враховує не лише цифри чи букви, а й мову, якою вони записані (наприклад, для кирилиці байти виділяються інакше). Прогноз записано вже після запуску програми.

Лабораторна пропонує замінити `int uninitialized;` на `int large_array[100000];`. Але `main` друкує `uninitialized`, і після заміни програма б не скомпілювалася. Тому я **додав** `large_array` поряд зі старою змінною. Для `.bss` ефект той самий.

| | text | data | bss | розмір файлу |
|---|---|---|---|---|
| До | 1450 | 608 | 16 | 16144 bytes |
| Після | 1450 | 608 | 400072 | 16176 bytes (16K) |

`bss` виріс приблизно на 400 000 bytes (100000 × 4 bytes, плюс трохи на вирівнювання), `text` і `data` не змінилися. Файл виріс лише на 32 bytes.

**Чому так.** `.bss` — секція `NOBITS`. У файлі зберігається лише інформація, що треба зарезервувати стільки-то нульових байтів. Самих байтів немає. Під час запуску OS виділяє цю пам'ять і заповнює нулями. Тому розмір процесу у пам'яті і розмір файлу не збігаються. Невеликий приріст файлу (32 bytes) — це, імовірно, службові дані про новий symbol, а не сам масив.

**Результат прогнозу:** сам прогноз «ні» збігся: файл виріс лише на 32 bytes. Але моє пояснення було хибним. Мова символів і кирилиця тут ні до чого, `large_array` не є текстом. Справжня причина в тому, що `.bss` має тип `NOBITS` і у файлі зберігається лише розмір, а не нульові байти.

## 6. `ldd` та dynamic linking

```
$ ldd program
	linux-vdso.so.1 (0x00007ffc8b3fe000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007750c12a2000)
	/lib64/ld-linux-x86-64.so.2 (0x00007750c14c3000)
```

`program` залежить від `libc.so.6`: реалізація `printf` лежить там, а не у нашому файлі (в `program.o` вона була `U`). `/lib64/ld-linux-x86-64.so.2` — dynamic loader (interpreter, його видно й у `file program`). Під час старту він підвантажує потрібні бібліотеки та зв'язує виклики. `linux-vdso.so.1` — спеціальна бібліотека, яку надає саме ядро.

Dynamic linking дозволяє багатьом програмам використовувати одну копію `libc` у пам'яті й на диску. Executable лишається малим, а оновлення бібліотеки не потребує перекомпіляції програм.

## 7. Прогноз і результат розміщення змінних

Прогноз я записав уже після запуску програм, тому він не є «чистим» прогнозом. Ось що я передбачив і що вийшло насправді:

| Об'єкт | Мій прогноз | Результат | Збіг? |
|---|---|---|---|
| main | code | code (`.text`) | так |
| global_initialized | data | data (`.data`) | так |
| global_uninitialized | heap | bss (`.bss`) | ні |
| static_initialized | | data | data (`.data`) | так |
| static_uninitialized | data | bss (`.bss`) | ні |
| local | stack | stack | так |
| dynamic allocation | bss | heap | ні |

Вивід `./memory` (запуск 1):

```
Function main: 0x5e2b6eb1a1a9
Global initialized: 0x5e2b6eb1d010
Global uninitialized: 0x5e2b6eb1d01c
Static initialized: 0x5e2b6eb1d014
Static uninitialized: 0x5e2b6eb1d020
Local variable: 0x7ffee0eeaecc
Dynamic allocation: 0x5e2b97e8c2a0
```

**Інтерпретація адрес.**

- Чотири змінні (`global_*`, `static_*`) стоять поруч: `...d010`, `...d014`, `...d01c`, `...d020`. Порядок збігається з `sections`: спершу ініціалізовані (`.data`), потім неініціалізовані (`.bss`). Ці дві секції стоять поруч у пам'яті.
- `main` (`...a1a9`) лежить в іншій області, ближче до початку образу програми (code, `.text`).
- `local` (`0x7ffe…`) далеко від глобальних змінних: це stack, зовсім інша область.
- `dynamic` (`0x5e2b97e8c2a0`) лежить вище програмного образу, але не поруч із глобальними змінними. Від `local` вона теж дуже далеко. Це heap.
- Висновок: address space поділений на окремі області (code, data/bss, heap, stack), а не є єдиним суцільним шматком.

## 8. Концептуальна memory map (`memory_map.c`)

Адреси з одного запуску `./memory_map`:

```
Code/function: 0x64664b2951a9
Global initialized: 0x64664b298010
Global uninitialized: 0x64664b29801c
Static initialized: 0x64664b298014
Static uninitialized: 0x64664b298020
String literal: 0x64664b296004
Heap allocation: 0x6466794412a0
Stack/local: 0x7ffdca0f38b4
```

```
Higher addresses
┌──────────────────────────────────────────┐
│ Stack                                    │
│ local                 0x7ffdca0f38b4     │
├──────────────────────────────────────────┤
│ ...                                      │
├──────────────────────────────────────────┤
│ Heap                                     │
│ malloc allocation     0x6466794412a0     │
├──────────────────────────────────────────┤
│ ...                                      │
├──────────────────────────────────────────┤
│ .bss                                     │
│ static_uninitialized  0x64664b298020     │
│ global_uninitialized  0x64664b29801c     │
├──────────────────────────────────────────┤
│ .data                                    │
│ static_initialized    0x64664b298014     │
│ global_initialized    0x64664b298010     │
├──────────────────────────────────────────┤
│ .rodata                                  │
│ "Hello, memory map"   0x64664b296004     │
├──────────────────────────────────────────┤
│ .text                                    │
│ example_function      0x64664b2951a9     │
└──────────────────────────────────────────┘
Lower addresses
```

Спостереження:

- `.text`, `.rodata`, `.data` і `.bss` програми лежать у сусідніх сторінках: від `0x…5000` до `0x…8000`. Це образ самого executable.
- Порядок збігається з тим, що було в `readelf`: `.text` < `.rodata` < `.data` < `.bss`. Адреси `…8010` та `…8014` (`.data`) ідуть перед `…801c` та `…8020` (`.bss`).
- Heap (`0x6466794412a0`) лежить значно вище образу програми, на відстані приблизно 0x2E1A9000 bytes, і окремо від нього.
- Stack (`0x7ffd…`) лежить дуже високо, далеко від heap та образу програми.
- Це концептуальна карта, а не точна карта всього Linux address space: між областями є великі проміжки.

## 9. Результати stack / heap

**Чи однакові адреси при кожному запуску (`./memory`, 3 запуски).**

| | main | global_initialized | local | dynamic |
|---|---|---|---|---|
| Run 1 | 0x5e2b6eb1a1a9 | 0x5e2b6eb1d010 | 0x7ffee0eeaecc | 0x5e2b97e8c2a0 |
| Run 2 | 0x5d50a265c1a9 | 0x5d50a265f010 | 0x7ffd0a15173c | 0x5d50ba27f2a0 |
| Run 3 | 0x5801e17dd1a9 | 0x5801e17e0010 | 0x7fff7f53e1fc | 0x5801f4db02a0 |

Ні, адреси щоразу різні. Це не помилка, а ASLR. Цікаво, що закінчення адрес (`...1a9`, `...010`, `...2a0`) однакові в усіх запусках: випадково змінюється лише база, а взаємне розташування об'єктів у програмі лишається тим самим.

**Stack (`./stack`):**

```
level1: &a = 0x7ffecfbc3354
level2: &b = 0x7ffecfbc3334
level3: &c = 0x7ffecfbc3314
```

Адреси змінюються при кожному вкладеному виклику. Вони **зменшуються** на 0x20 (32 bytes) на кожному рівні, тобто stack росте у бік менших адрес. Це узгоджується з моделлю stack для x86-64 у моєму середовищі. Як універсальне правило для всіх ISA чи ABI я це не розглядаю.

**Heap (`./heap`):**

**Прогноз:** ні, адреси `a, b, c` не будуть послідовними (прогноз записано після запуску).

```
a: 0x617754bc22a0
b: 0x617754bc22c0
c: 0x617754bc22e0
```

У цьому запуску адреси зростають з кроком 0x20 (32 bytes), хоча `sizeof(int)` лише 4. Allocator виділяє блоки з власними службовими даними та вирівнюванням. Це не означає, що `malloc` завжди повертає послідовні адреси: це особливість конкретного allocator у цьому середовищі. Головне, що heap-адреси (`0x6177…`) в іншій області, ніж stack (`0x7ffe…`).

**Результат прогнозу:** прогноз не збігся. У цьому запуску адреси йшли одна за одною з рівним кроком 0x20. Але це особливість allocator у цьому середовищі: `malloc` не гарантує послідовність.

## 10. Відповіді на контрольні питання

1. **Основні етапи.** Preprocessing → compilation → assembly → linking → (завантаження в пам'ять і виконання як process).
2. **Preprocessor** обробляє директиви (`#include`, `#define`): підставляє вміст заголовків і макроси. Результат: `.i`, усе ще C.
3. **Compiler** перетворює C на Assembly (`.s`) для конкретної ISA.
4. **Assembler** перетворює Assembly на machine code в object file (`.o`).
5. **Object file** містить machine code, дані, таблицю symbols та інформацію для linker (relocations). Адреси ще не остаточні.
6. **Linker** з'єднує object files і бібліотеки, зв'язує посилання на symbols з їхніми визначеннями й створює executable.
7. **`undefined reference`** означає, що linker не знайшов визначення symbol, який використовується (як `calculate` у `main.o`).
8. **Symbol** — ім'я функції або змінної в object file/executable. Воно або визначене тут (`T`, `D`, `B`), або потрібне ззовні (`U`).
9. **Header** містить оголошення (прототипи, типи) для compiler. **Library** містить скомпільовану реалізацію для linker. Header сам по собі `printf` не реалізує.
10. **ELF** — формат виконуваних та об'єктних файлів у Linux (заголовок, секції, сегменти).
11. **`.text`** — machine code програми (у нас `add`, `main`).
12. **`.rodata`** — дані тільки для читання: константи, рядкові літерали (`"Computer Systems"`, рядок формату `printf`).
13. **`.data`** містить ініціалізовані глобальні/статичні змінні, їхні значення зберігаються у файлі. **`.bss`** містить неініціалізовані (нульові) змінні, у файлі лише розмір, місце виділяється під час запуску.
14. Бо `.bss` не зберігає вміст у файлі, лише його розмір. Файл `sections` виріс лише на 32 bytes, а `bss` — на ~400 000.
15. **`ldd`** показує, від яких shared libraries залежить executable та де вони знаходяться (`libc.so.6`, loader).
16. **Executable** — файл на диску. **Process** — запущена програма з PID, власним address space, стеком, heap. З одного файлу можна запустити кілька процесів.
17. **Local variables** концептуально лежать у stack. У нашому експерименті це адреси `0x7ffe…`.
18. **Heap** використовується для динамічної пам'яті (`malloc`), розмір та час життя якої визначаються під час виконання.
19. **Stack** автоматично виділяється та звільняється разом з викликами функцій, росте у бік менших адрес (у нашому середовищі). **Heap** керується програмою явно (`malloc`/`free`) і лежить в іншій області address space.
20. Через **ASLR**: OS випадково обирає бази областей процесу при кожному запуску, це ускладнює експлуатацію вразливостей. У нас змінювалися `main`, глобальні змінні, local та heap.
21. **Temporal locality**: якщо до даних щойно зверталися, велика ймовірність, що до них звернуться знову незабаром (наприклад, змінна циклу).
22. **Spatial locality**: якщо звернулися до адреси, ймовірно, скоро звернуться до сусідніх адрес (наприклад, послідовний прохід по масиву).

## 11. Висновок

У цій роботі я окремо виконав усі етапи від `program.c` до executable та побачив, чим відрізняються `.c`, `.i`, `.s`, `.o` та ELF. На linker error `undefined reference` я побачив роль symbols і linker. В ELF знайшов `.text`, `.rodata`, `.data`, `.bss` і перевірив, де лежать мої змінні. Експеримент з `large_array` показав, що `.bss` майже не збільшує файл, зате збільшує пам'ять процесу. Адреси в `memory`, `stack` та `heap` показали, що code, data/bss, heap і stack розташовані в різних областях address space. А ASLR змінює їх при кожному запуску.і