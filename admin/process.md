# How is used
## the real user ID (ruid) of a process?

It identifies  the real  owner of  the process and  affects the  permissions for
sending signals.

A process  without superuser privileges may  signal another process only  if the
sender's ruid (or euid) matches receiver's ruid (or suid).
Because a child  process inherits its ruid  from its parent, a  child and parent
may signal each other.

<https://en.wikipedia.org/wiki/User_identifier#Real_user_ID>

## the effective user ID (euid) of a process?

It's used for most access checks.
It's also used as the owner for files created by that process.

<https://en.wikipedia.org/wiki/User_identifier#Effective_user_ID>

## the saved user ID (suid) of a process?

It's  used when  a process  running with  elevated privileges  needs to  do some
unprivileged work temporarily; changing euid  from a privileged value (typically
0) to some unprivileged value causes the privileged value to be stored in suid.

Later, the process's euid  can be set back to the value stored  in suid, so that
elevated privileges can be restored.

<https://en.wikipedia.org/wiki/User_identifier#Saved_user_ID>

##
# How to print
## all the limits imposed on the resources available to the shell and the processes started by it?

    $ ulimit -a

See `man bash /ulimit` for more info.

##
## the maximum size of a core file created when a process crash?

    $ ulimit -c

By default, this command will probably output 0.
It means that if a process crashes, and tries to dump a core file, it won't be able to.

### How to remove this limit?

    $ ulimit -c unlimited

The effect of this command doesn't persist beyond the life of the shell where it's run.

##
## apport

<https://stackoverflow.com/q/2065912/8243465>

Question intéressante: “core dumped - but core file is not in current directory?”

---

    /proc/sys/kernel/core_pattern

Par défaut, un  fichier “core dump“ est  nommé 'core', mais ce  peut être changé
via un template défini dans `/proc/sys/kernel/core_pattern`.
Atm, le mien contient:

    |/usr/share/apport/apport %p %s %c %P

Ici, le 1er caractère est un pipe.
Ça indique  au kernel qu'il ne  doit pas écrire  le “core dump“ dans  un fichier
mais sur l'entrée standard du programme `apport`.

On peut aussi accéder à ce paramètre via `$ sysctl kernel.core_pattern`.
Et on peut aussi – sans doute – le modifier en passant `-w` à `sysctl(8)`.

---

Les items  `%` sont des  spécificateurs automatiquement remplacés  par certaines
valeurs.
Pour plus d'infos lire `man core`.

---

`apport` vérifie que le  binaire fait partie d'un paquet, et si  c'est le cas il
génère un rapport qu'il envoit à un bug tracker.

Si le binaire  ne fait pas partie  d'un paquet (pex compilé  en local), `apport`
simule  ce que  le kernel  aurait fait,  à savoir  écrire le  core dump  dans un
fichier du CWD du processus.

Si un utilisateur  a besoin du fichier  core pour générer un  backtrace, il faut
distinguer 3 cas de figure:

   - le binaire fait partie d'un paquet:

    `apport` a généré un rapport dans `/var/crash`

   - le binaire ne fait pas partie d'un paquet, et la taille max d'un fichier
     core est limitée à 0 blocks (limite par défaut; vérifiable via
     `ulimit -a | grep core`):

     il n'y a pas de fichier core

   - le binaire ne fait pas partie d'un paquet, et la taille max d'un fichier
     core n'est pas limitée (ou a une limite suffisamment élevée;
     `ulimit -c unlimited`):

     `apport` a  écrit le core dump  dans un fichier  du CWD du processus  qui a
     crashé

En cas  de crash  d'un binaire  faisant partie d'un  paquet, `apport`  génère un
rapport dans un fichier de ce dossier.
Pex, s'il s'agit de Vim, il l'écrira dans:

    /var/crash/_usr_bin_vim.gtk.1000.crash
                                │
                                └ User ID de l'utilisateur au nom duquel tourne le processus?

Ce rapport contient différentes informations, entre autres un backtrace.
Ce dernier n'est sans doute pas très utile  si le binaire qui a crashé ne génère
pas d'infos de déboguage.

---

    /var/log/apport.log.1

Fichier dans lequel le système logue l'activité de `apport`.

Utile qd  on ne  trouve pas où  `apport` a  écrit le core  dump d'un  binaire ne
faisant pas partie d'un paquet.

## gdb

Si aucun  fichier `core`  n'est créé à  l'issu du crash,  reproduit le  crash en
ayant lancé le processus via `sudo`.

Et lit le contenu de `/var/log/apport.log`.
Il se peut qu'il contienne un message d'erreur expliquant pourquoi le `core` n'a
pas été dumpé, ou bien il peut fournir le chemin vers lequel il a été dumpé.


         ┌ quiet: pas de messages d'intro / copyright
         │
    gdb -q build/bin/nvim core
           ├─────────────────┘
           └ Lance le  binaire nvim  en spécifiant  un fichier
             `core` pour analyser un précédent crash.

            ┌ exécute automatiquement la commande GDB qui suit (ici `bt`)
            │
    gdb -q -ex bt build/bin/nvim core
               │
               └ affiche le backtrace de  toutes les stack frames (taper `help
                 bt` dans gdb pour + d'infos)

                ┌ appliquer  la commande qui suit  (ici `bt`) à
                │ tous les threads neovim est multi-thread
                ├──────────────┐
    gdb -q -ex 'thread apply all bt full' build/bin/nvim core
                                    │
                                    └ qualificateur qui demande à afficher les
                                      valeurs des variables locales


         ┌ n'exécute aucune commande d'un fichier d'initialisation `.gdbinit`
         │
    gdb -n -ex 'thread apply all bt full' -batch nvim core >backtrace.txt
                                           │
                                           └ mode batch (!= interactif):
                                                  exécute les commandes demandées et affiche leur sortie
                                                  dans le terminal
                                             -batch implique `-q`


Générer un backtrace à partir d'un fichier “core dump“.

Qd un  processus reçoit certains  signaux, il crashe  et génère un  fichier core
dump contenant une image de sa mémoire actuelle.
Cette image peut être utilisée par un debugger tq `gdb` pour inspecter l'état du
programme au moment où il s'est terminé.


Si le  crash concerne un  binaire compilé mais  non installé, il  faut remplacer
`nvim` par le chemin vers le binaire, typiquement:

    ./build/bin/nvim


La version `Release` ne génère pas d'informations de déboguage.
En  cas   de  crash,  il  vaut   donc  mieux  le  reproduire   avec  la  version
`RelWithDebInfo` et s'assurer  que la commande `gdb` invoque  bien cette version
de nvim.

See also: <https://github.com/neovim/neovim/wiki/FAQ#debug>

Now, it's:

    2>&1 coredumpctl -1 gdb | tee -a bt.txt
    thread apply all bt full

It doesn't need a core file, but you need to install the package `systemd-coredump`.

You can find some of these gdb commands by cloning the neovim wiki:

    $ git clone https://github.com/neovim/neovim.wiki

Then, search for ‘gdb’ in the commit logs:

    $ git log --all --source -p -S 'gdb' | vim -

See also: <https://wiki.archlinux.org/index.php/Core_dump>

---

Une frame est un ensemble de données associées à un appel de fonction.
Elle contient:

   - les arguments passés à la fonction

   - ses variables locales

   - son adresse d'exécution (≈ à quelle ligne de la fonction l'exécution se
     trouve ?)

On parle  de “stack frame“, car  une fonction peut  en appeler une autre,  et le
processus peut se  répéter, formant ainsi une pile sur  laquelle s'ajoute chaque
nouvelle frame.
La frame  associée à  la fonction où  l'exécution se trouve,  est dite  “la plus
profonde“ (innermost).

Ce qui caractérise une stack n'est pas son implémentation (liste ou autre), mais
son interface: on ne peut que “push“ ou “pop“ un item sur la stack.
<https://en.wikipedia.org/wiki/Stack_(abstract_data_type)#Implementation>

---

How to generate a core file on-demand?

Start your process, and get its pid.
Then, run:

    $ gdb -p PID
    generate-core-file

<https://wiki.archlinux.org/index.php/Core_dump#Making_a_core_dump>

##
##
##
# How to kill the process responsible for a GUI window, without knowing its pid?

    $ xkill
    # hover your cursor on the window
    # left-click on it

The only thing that `xkill(1)` does, is to close the connection to the X server.
There's no  guarantee that the application  will abort nicely, or  even abort at
all.
Many existing applications do indeed abort when their connection to the X server
is closed, but some can choose to continue.

##
##
##
# How to get
## the environment of the Vim process?  (2)

    $ tr '\0' '\n' </proc/$(pidof vim)/environ

Note that this shows the environment as it was when the process was spawned.
Any change the process might have made to its environment won't be visible:
<https://serverfault.com/a/79463>

---

Alternatively, start `htop`, select the Vim process, and press `e`.

## the list of processes whose name is 'firefox'?

    $ pidof firefox

## the list of processes whose name matches the regex `fire*`?

    $ pgrep 'fire*'

## the tree of processes from systemd down to the Vim process?

    $ pstree -lsp $(pidof vim)
               │
               └ show parent processes of the specified process

##
##
##
# Concepts
## Session
### What's a login session?

The period of activity between a user logging in and logging out of the system.

### How is it implemented when there's no graphical user interface?

With a  kernel session: a  collection of process  groups with the  logout action
managed by a session leader.

### How is it implemented when an X display manager is used (like lightdm)?

With the lifetime of a designated user process that the display manager invokes.

### In a kernel session, what is the session leader?

A  process which  interacts with  the controlling  terminal to  ensure that  all
programs are terminated when a user “hangs up” the terminal connection.

On our machine, atm, it seems the session leader is `upstart`:

    $ pstree -lsp $(pgrep upstart | head -n1)
    systemd(1)---lightdm(980)---lightdm(1086)---upstart(1096)...˜
                                                             │˜
                  all the programs we start during a session ┘˜
                  are children of this `upstart` process˜

If the  session leader  is absent,  the processes  in the  terminal's foreground
process group are expected to handle hangups.

### When does a session end?

When the user logs out or exits their interactive shell.

### What happens then?

It terminates the session leader process.
The shell  process then  sends SIGHUP  to all  jobs, and  waits for  the process
groups to end before terminating itself.

###
## Process group
### What is a process group?

A collection of one or more related processes.

Any shell command starts a process group of one or several processes.
There  may be  several because  the  command may  be  a compound  command, or  a
pipeline.
Besides a process may spawn child processes.

#### What is it used for?

It's used to send a signal to several related processes simultaneously.

###
### What is the purpose of the foreground process group of a terminal?

It determines  what processes may  perform I/O to and  from the terminal  at any
given time.

It's  also the  process  group to  which  the tty  device  driver sends  signals
generated by keyboard interrupts, notably C-c, C-z and C-\.

#### Who sets it?

The shell.

It partitions  the command pipelines that  it executes into process  groups, and
sets  what process  group is  the foreground  process group  of its  controlling
terminal.

###
## Special processes
### What's the term for a process
#### which has started (directly or indirectly) another process?

An ancestor process.

#### which has been started (directly or indirectly) by another process?

A descendant process.

###
### What's an orphan process?

A process whose parent has finished or been terminated, but is still running.

### What does “re-parenting” mean?

An operation  performed automatically by  the kernel, which consists  in setting
the parent of an orphan process to init (or a subreaper).

The term can be used in a sentence like so:

    The orphan process has been re-parented to the init process.
                                ^---------^ ^^

### What's a subreaper process?

A subreaper fulfills the role of init for its descendant processes.
Upon termination of a  process that is orphan and marked  as having a subreaper,
the nearest  ancestor subreaper will receive  SIGCHLD and be able  to wait(2) on
the orphan to discover its termination status.

For more info, see:

    man 2 prctl

<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=ebec18a6d3aa1e7d84aab16225e87fd25170ec2b>

###
### What does it mean for a parent process to wait(2) on a child?

It means that the parent calls the  system call `wait()` to get information when
the state of one of its child changes.

A state change can be:

   - the child has terminated
   - the child was stopped by a signal
   - the child was resumed by a signal

For more info, see:

    man 2 wait

### Why is it important to do so?

When a  child terminates,  performing a  wait allows the  system to  release the
resources associated with it.

### What happens to a terminated child which is not waited on?

It remains in a zombie state.

##
# ps
## What's the effect of the option?
### `a`?

When you use  a BSD-style option – whose  name is not prefixed by  `-` – `ps(1)`
only displays processes owned by the current user.

`a` removes this restriction.

### `x`?

When you use a BSD-style option, `ps(1)` only displays processes who have a tty.

`x` removes this restriction.

### `f`?

It draws some ASCII art to represent the parent-child relationship between processes.

The 'f' is for "forest".

### `u`?

It selects information and format them according to a predefined user-oriented format.

---

Other similar options  exist to print information according  to other predefined
formats, highlighting various characteristics of processes:

    ┌───┬───────────────────────┐
    │ l │ BSD long format       │
    ├───┼───────────────────────┤
    │ s │ signal format         │
    ├───┼───────────────────────┤
    │ v │ virtual memory format │
    ├───┼───────────────────────┤
    │ X │ register format       │
    └───┴───────────────────────┘

### `w`?

Long lines are wrapped.

You can also use `less(1)` to read long lines.

##
## How to only print the effective user, pid, tty, state and command of all current processes?

    $ ps axfo user,pid,tty,stat,args
            ├──────────────────────┘
            └ user-defined format

`o`/`-o` is  an option which  you can use to  specify how which  information you
want to see, and how they should be formatted.
See `man ps /STANDARD FORMAT SPECIFIERS` for the full list of keywords you can use.

### The header of the tty column is `TT`.  How to make `ps(1)` write `TTY` instead?

You  can populate  a  column header  with  an arbitrary  text  by suffixing  the
relevant keyword with `=mytext`:

    $ ps axfo user,pid,tty=TTY,stat,args
                          ^--^

#### And how to make the column 13 cells wide?

Specify the desired width after a colon:

    $ ps axfo user,pid,tty:13=TTY,stat,args
                          ^-^

---

Note that if  you specify `=mytext`, `:number` must precede,  otherwise it would
be wrongly interpreted as being part of the text in the column header:

    ✘
    $ ps axfo user,pid,tty=TTY:13,stat,args
                              ^-^
                              would be  interpreted literally  as being  part of
                              the column header for the tty keyword

###
### How to suppress the output of the header line?

Empty every column header with an equal sign.

This is especially useful when you  only want the information about one process,
and the header is just noise:

    $ ps o pid=,tty=,stat=,args= -p $(pidof vim)
              ^    ^     ^     ^

##
### How to also include the name of the kernel function where a process is sleeping?

Include the `wchan` keyword in your format:

    $ ps xo pid,wchan,args
                ^---^

#### What does it mean for `-` to be printed instead of a kernel function name?

The process is not sleeping.

##
## How to only print
### the processes whose effective user ID is root?

Use the `-u` option:

    $ ps -u root

### the processes whose pid are 12, 34 and 56?

Use the `-p` option:

    $ ps -p 12,34,56
         ^^

### the 10 most memory-consuming processes?

Use the `--sort` option, pass it the `rss` keyword, and pipe the output of `ps(1)` to `head(1)`:

    $ ps aux --sort -rss | head -n11
                    │
                    └ print the processes in a descending order

                      by default, or with a `+` sign, the processes would be
                      displayed in an ascending order

##
## What do the numbers in the TIME column mean?

They stand for how much cpu time the processes have consumed thus far.
They do NOT stand for how long the processes have been running.

## Where does the name of the `ps(1)` command come from?

Process Status

##
# pidof, pgrep
## To what is compared the argument of
### pidof?

The names of the executables which have been run to start the running processes.

### pgrep?

The names of the running processes.

### pgrep -fl?

The full command-lines which have been run to start the running processes.

##
## How to get the *name* of
### all processes started from a command-line containing 'firefox'?

    $ pgrep -fl firefox
             ││
             │└ *l*ist the process name as well as the process ID
             └ matches 'firefox' against the *f*ull command-line

### a given process from its PID?

    $ ps o comm= -p PID

---

Note that it's not clear how a process name is chosen.

It may be that a process and/or its parent can set the name arbitrarily:
<https://unix.stackexchange.com/q/279782/289772>

    $ pgrep -fl firefox
    3111 firefox˜
    3157 Web Content˜
    3202 WebExtensions˜
    7177 Web Content˜

###
## What are the three differences between `pidof(8)` and `pgrep(1)`?

First, if several pids match the argument:

   - `pidof(8)` prints them on a single line, separated by spaces
   - `pgrep(1)` prints each of them on a dedicated line

Second, `pidof(8)`  parses its  argument as a  *literal* name,  while `pgrep(1)`
parses it as an ERE *regex*.

Third, `pidof(8)` matches its argument against the full name of the *executable*,
while `pgrep(1)` matches it against the *name of the process*.

### Why does `$ pgrep firefox` only output one pid, while `$ pidof firefox` output several?

You probably have several firefox processes,  but only one contains 'firefox' in
its name:

    $ ps xo fname | grep firefox
    firefox˜

So `pgrep(1)` only prints the pid of the latter.
However, all of them were started from the 'firefox' executable:

    $ ps xo args | grep firefox
    /usr/lib/firefox/firefox ...˜
    ...˜
    ...˜

So `pidof(8)` prints all of their pids.

#### How to make `pgrep(1)` print all the firefox processes?

Pass it the `-f` option so that  it matches the regex 'firefox' against the full
command-line of each process, and not just their name.

##
## How to limit the output of `pgrep(1)` to the processes
### of the root user?

Use the `-u root` argument.

Only the euid is considered, not the ruid.

### whose parent has the PID 123, 456 or 789?

Use the `-P 123,456,789` argument.

##
# Signals
## In which command(s) do I need to
### write a signal name in uppercase?

When passed as an argument to `trap` in zsh.

In all other cases, you can write a signal name in lowercase too.
That is, when the signal name is passed as an argument to:

   - `kill` in sh/bash/zsh
   - `trap` in sh/bash

### shorten a signal name by removing its `SIG` prefix?

`kill` and `trap` but only in the sh shell.

In bash and zsh, you can use the full name of a signal (e.g. SIGTERM).
Although, for consistency, and because it's shorter, I would recommend using the
shortened version of a signal name.

##
## When does a process receive
### SIGWINCH?

When the size of its graphical window (could be a terminal emulator window) has changed.

### SIGCHLD?

When one of its children has stopped or died.

### SIGPIPE?

When it tries to write to a pipe while the reading end has been closed.

The idea is that if you run `$ foo  | bar` and `bar` exits, `foo` gets killed by
a SIGPIPE (sent by the shell?).

It doesn't necessarily raise an error.
It  does only  if the  signal  handler for  SIGPIPE  has been  set to  `SIG_IGN`
(ignore); in that case, an error is reported via the exit status of write(2).

<https://unix.stackexchange.com/a/482254/289772>

##
## What's the number of the default signal sent by `kill` to a process?  What's its name?

    15

    SIGTERM
       ^--^
       TERMination

### What are the 4 other signals I can send to kill a process?  (name + number)

    ┌─────────┬────────┬─────────────────────────────────────────┐
    │ name    │ number │ meaning                                 │
    ├─────────┼────────┼─────────────────────────────────────────┤
    │ SIGHUP  │ 1      │ Hangup detected on controlling terminal │
    │         │        │ or death of controlling process         │
    ├─────────┼────────┼─────────────────────────────────────────┤
    │ SIGINT  │ 2      │ Interrupt from keyboard                 │
    ├─────────┼────────┼─────────────────────────────────────────┤
    │ SIGQUIT │ 3      │ Quit from keyboard                      │
    ├─────────┼────────┼─────────────────────────────────────────┤
    │ SIGKILL │ 9      │ Kill signal                             │
    └─────────┴────────┴─────────────────────────────────────────┘

#### How are they sorted, from the least effective to the most effective?

    int(2) < hup(1) ≈ term(15) < quit(3) < kill(9)

<https://unix.stackexchange.com/a/251267/289772>

##
## What happens when a process tries to read from the terminal or write to it, outside the foreground process group?

The tty device driver sends to it the SIGTTIN (read) or SIGTTOU (write) signal.
Unless caught, these signals make the process stop.

Shells often  override the  default stop  action of  SIGTTOU so  that background
processes deliver their output to the controlling terminal by default.

## How do daemons usually react to SIGHUP?

They reload their config.

   > SIGHUP   also   has   a   completely  different   conventional   meaning   for
   > non-user-facing applications (daemons), which is to reload their configuration
   > file.

<https://unix.stackexchange.com/a/251267/289772>

As an example, see `man xbindkeys /HUP`.

###
## How to send – in a single command – SIGKILL to
### the processes whose pid are 123 and 456?

    $ kill -kill 123 456

### the jobs whose jobspec are %1, %2 and %3?

    $ kill -KILL %1 %2 %3

### the process group whose PGID is 123?

Use the PGID prefixed by `-`:

    $ kill -KILL -123
                 ^

---

If you wanted to send the default  signal (SIGTERM), you would need `--` to mark
the end of the optional arguments:

    $ kill -- -123
           ^^

##
## getting info
### Where can I find more info about the different signals that I can send to a process?

    man 7 signal /Standard signals

###
### How to get
#### the list of all possible signal names?

    $ kill -l

#### the number of the signal INT?

    $ kill -l int
    2˜

#### the name of the signal 8?

    $ kill -l 8
    FPE˜

#### the name of the signal which has terminated or stopped my process?

    $ kill -l <exit status>

Example:

    $ sleep
    C-c
    # the exit status is 130

    $ kill -l 130
    INT˜

###
### What does it mean for a process to “catch” a signal?

The process has registered a signal handler which will be automatically run when
the signal is received.
This handler will determine the behavior of the process, which may be completely
different than the  default behavior of the  process if the signal  had not been
caught.

### How to know which signals a given process block/ignore/catch?

    $ cat /proc/PID/status | grep -E '^Sig(Blk|Ign|Cgt):'
                                           │   │   │
                                           │   │   └ caught signals
                                           │   └ ignored signals
                                           └ blocked signals

The numbers on the right are bitmasks written in hexadecimal.
To understand their meaning, you must convert them in binary:

    $ echo 'ibase=16;obase=2;ABC123' | bc
    101010111100000100100011˜

The *index* of each non-zero bit stands for the number of a signal.

So, for example, if you have this bitmask:

    SigCgt: 00000000280b2603

It can be converted in binary, and interpreted like so:

    SigCgt: 101010111100000100100011
                              │   ││
                              │   │└ the signal 1 is caught (SIGHUP)
                              │   └ the signal 2 is caught (SIGINT)
                              └ the signal 6 is caught (SIGUSR1)
            ...


To  automate the  binary  conversion and  interpretation, we  have  a script  in
`~/bin/signal.sh`.
To use it, invoke it  on the command-line and pass it the pid  of a process as a
parameter:

    $ signal.sh $(pidof vim)

For more info, see:

<https://unix.stackexchange.com/a/85365/289772>

##
## keypress-generated signals
### What's the name of the signal generated by the keypress
#### C-c?

    SIGINT

#### C-\?

    SIGQUIT

#### C-z?

    SIGTSTP

### What happens when I press
#### C-c?

The SIGINT signal is sent to the foreground process group.
By default, SIGINT makes processes go back to the main loop.

Example:

    $ vim
    :sil call system('sleep 123')
    " can't interact with Vim anymore
    C-c
    " can interact again (presumably because Vim went back to its main loop)

#### C-\?

The SIGQUIT signal is sent to the foreground process group.
By default, SIGQUIT makes processes quit immediately.

#### C-z?

The SIGTSTP signal is sent to the foreground process group.
By default, SIGTSTP makes processes stop, and control is returned to the shell.

##
### How to change the character which I have to send to interrupt a process (by default C-c)?

    $ stty intr '^k'
                  │
                  └ case insensitive

Technically,  this command  tells the  terminal device  that the  character that
causes a SIGINT to be sent to the foreground job is `C-k`.

### How to tell the terminal device to never send SIGINT, SIGQUIT, SIGTSTP?

    $ stty -isig

##
## suspension
### Which signal(s) can I send to
#### suspend a job?

SIGTSTP or SIGSTOP.

Mnemonic for SIGTSTP: Typing SToPped

##### What's the difference between them?

SIGTSTP allows the process  to suspend gracefully (i.e. it can  use some time to
organize itself), and can be caught or ignored.

SIGSTOP forces  the process  to suspend  immediately, can't  be caught,  and may
leave the process in an unstable state (i.e. can't be resumed).

#### resume a suspended job in the background?

SIGCONT

###
### How to suspend the current job, which runs in the background, with `kill`?

    $ kill -tstp %

#### Without `kill`?

    $ fg
    C-z

##
### What should I do before suspending a GUI program with `kill` or `C-z`?

Minimize it.
Otherwise its frozen window will take valuable space on your desktop, and you'll
get confused if you try to interact with it (because it won't respond).

##
## traps
### How to list all the traps set in the current shell?

    $ trap

### How to make all future commands run in the shell ignore a given signal?

Install a trap running an empty command.

For example, to make all commands ignore SIGHUP:

    $ trap '' HUP
    $ sleep 1234

Then, from another terminal:

    $ kill -HUP $(pidof sleep)

The sleep process won't terminate.

---

TODO: For some  reason, you  can't set  a trap to  make a  command run  from zsh
ignore SIGTERM.  Why?

### What does resetting a trap mean?

The specified signal is reset to its original disposition (the value it had upon
entrance to the shell).

#### How to do it?

Use the special argument `-`.

    $ trap - sigspec

---

Example:

    $ trap '' HUP
    $ trap
    trap -- '' HUP˜

    $ trap - HUP
    $ trap
    ''˜

In zsh, you can reset all traps by omitting the sigspec:

    $ trap -

##
# strace
## What does `strace(1)` do?

It runs the specified command until it exits, and intercepts the system calls of
the process started by the command, as well as the signals which are received by
the latter.

### How is it useful?

Since  system calls  and  signals  are events  that  happen  at the  user/kernel
interface, a close examination of this boundary is very useful for:

   - bug isolation
   - sanity checking
   - capture race conditions

It's also useful  to solve an issue with  a program for which the  source is not
readily available, since you don't need to recompile it in order to trace it.

##
## Where is the output of `strace(1)` written?

By default, on standard error.

### How to redirect it into a file?

Use the `-o` option.

##
## What does `123` mean in `system_call(args) = 123`?

It's the return value of the system call.

##
## How to start Vim, and trace all its system calls and received signals?

    $ strace -o log vim
             ├────┘
             └ write the output in the `log` file

---

This kind of command is especially useful with small processes, such as `localectl`:

    $ strace -o log localectl list-keymaps

Because the log will be short, and it will be easy to find the cause of an issue.
It's less useful with big processes such as Vim or zsh, but you can still try...

## How to trace an existing Vim process?

    $ strace -o log -p $(pidof vim) &!
                    ^-------------^

    $ less +F log

---

FIXME: For some  reason, `less(1)`  doesn't update the  log file  as `strace(1)`
writes into it.

MWE:

    $ echo test >>/tmp/log

`less(1)` doesn't show `test`.

`$ tail -f log` doesn't suffer from this issue.

    $ echo one >/tmp/file
    $ less +F /tmp/file

    # from another terminal
    $ echo two >>/tmp/file

Here,  `$ less  +F` works  as  expected; so  the issue  probably lies  somewhere
between `strace(1)` and `less(1)`.

##
## How to trace a process AND all its children?

    $ strace -o log -f firefox
                    ^^

Warning: This can create big files.

## How to log the system calls of each process in a dedicated file?

    $ strace -o log -ff firefox
                    ^-^
                    each process's trace is written to `log.<pid>`
                    where pid is the pid of the process

##
## How to make `strace(1)` add an absolute timestamp before each system call?

    $ strace -o log -t vim
                    ^^

### How about a relative timestamp?

    $ strace -o log -r vim
                    ^^

##
## How to trace only the system calls
### accessing files?

    $ strace -o log -e trace=file vim
                    ^-----------^

Useful when a  program fails because it can't find/read  its configuration file,
but doesn't tell you where it's supposed to be.

### `open()`?

    $ strace -o log -e trace=open vim
                    ^-----------^

### `open()` and `read()`?

    $ strace -o log -e trace=open,read vim
                    ^----------------^

##
## How to get statistics about the system calls (time, count, number of errors...)?

    $ strace -o log -c vim
                    ^^

##
## The maximum size of printed strings is 32 bytes!  How to get longer strings?

    $ strace -o log -s64 vim
                    ├──┘
                    └ truncate long strings after 64 bytes, instead of 32

This is  especially useful for  `write()` system  calls, because they  may write
long strings of text in files.

Note that filenames are not considered strings and are always printed in full.

##
# Issues
## My process is taking too much time!

    $ strace -o log -r <cmd>
    $ sort log >sorted_log

The lines at the bottoom should match the slowest system calls.
Try to understand why they take so much time...

---

For some reason, if you reverse the order of `sort(1)`, you lose Vim's syntax highlighting.

## I know that my command opens the file 'foo'.  But `$ strace -o log -e trace=open cmd` can't find it!

The process may use another similar system call:

   - openat
   - creat

See `man 2 open`.

So, to be more thorough, you should execute:

    $ strace -o log -e trace=open,openat,creat cmd
                             ^---------------^

It's also possible that the process started by your command spawns child processes.
And maybe it's one of them which opens 'foo'.
So, to be even more thorough, execute:

    $ strace -o log -e trace=open,openat,creat -f cmd
                                               ^^

##
# Todo
## Document how to start a job which doesn't stop when we close the shell.

I had this alias in the past:

    alias dropbox_restart='killall dropbox; ( "${HOME}/.dropbox-dist/dropboxd" & )'

The reason  why I started  dropbox from  a subshell was  to avoid the  job being
terminated when we left the shell from which we ran the alias.
When we start a job from a  subshell, it seems that it's immediately re-parented
to the session leader (here it was `upstart`).

May be relevant:

- <https://blog.debiania.in.ua/posts/2013-03-13-fun-with-bash-disown.html>
- <https://thinkiii.blogspot.com/2009/12/double-fork-to-avoid-zombie-process.html>

Make sure it's true.
Check whether  there're other ways to  start a job which  persists after leaving
the shell (`nohup`, `&!`, ...).
Which way is the most reliable?

Study the PGID and the file descriptors of a process in a subshell vs in a script.

Is a subshell interactive?

## Document `$ pkill -u toto`

Kills all processes whose EUID is toto.

## Document the process state code `I`

It means that the process is *i*dle.

- <https://stackoverflow.com/a/49407039/9780968>
- <https://elixir.bootlin.com/linux/v4.15.12/source/fs/proc/array.c#L135>

## Document how a child process dies

I think it calls exit(2) and returns its exit status to the kernel.
Then, the kernel:

   - terminates the child
   - closes all its open file descriptors
   - reparent its possible children to a subreaper or init

At that moment, the child is a  zombie, because it has terminated but the kernel
sill keeps some  info about it; notably,  its pid in the process  table, and its
exit status.

Then, the kernel sends SIGCHLD to the parent process.
The latter should then collect the exit status via wait(2).
Finally, the kernel removes the pid of the zombie from the process table.

See `man 2 exit`:

   > The  function _exit()  terminates the  calling process  "immediately".  Any
   > open file descriptors belonging to the  process are closed; any children of
   > the process are  inherited by process 1, init, and  the process's parent is
   > sent a SIGCHLD signal.

   > The value  status is returned to  the parent process as  the process's exit
   > status, and can be collected using one of the wait(2) family of calls.

## Document the difference between a task, a process and a thread

I  think   a  thread  is  a   lightweight  process  that  you   haven't  started
intentionally.
Instead,  it was  spawned by  another process,  probably to  improve performance
(i.e. by leveraging multi-core cpus to execute several unit of executions).

I think a task is a process  you've started (e.g. firefox), plus all the threads
it has itself spawned.

When you view an image such as:
<https://upload.wikimedia.org/wikipedia/commons/thumb/a/a5/Multithreaded_process.svg/1024px-Multithreaded_process.svg.png>
I think that the text "process" should be replaced by "task".
From the  kernel point  of view,  there's no (or  little?) difference  between a
process and a thread.
So saying that a process is a group of threads is misleading.
A *task* is a heavy process + a group of lightweight threads.

---

In `htop(1)`, you can view the number of tasks.
You can also hide/show the threads created by:

   - user processes by pressing `H`
   - kernel processes by pressing `K`

What is shown in the column PID of htop is not always a pid.
I think sometimes, it's a tid (thread id):

    $ ps -eLf | sed '/UID\|cmus/!d' | grep -v sed

Notice how all  the lines show the  same PID, but not the  same LWP (LightWeight
Process id); see `man ps /lwp\s*LWP`.

   > lwp         LWP       light weight process (thread) ID of the
   >                       dispatchable entity (alias spid, tid).  See tid
   >                       for additional information.

However, things are confusing, because it seems that a tid can appear as a pid:

   > tid         TID       the unique number representing a dispatchable
   >                       entity (alias lwp, spid).  This value may also
   >                       appear as: a process ID (pid); a process group ID
   >                       (pgrp); a session ID for the session leader
   >                       (sid); a thread group ID for the thread group
   >                       leader (tgid); and a tty process group ID for the
   >                       process group leader (tpgid).

Difference between PID and TID: <https://stackoverflow.com/a/8787888/9780968>

Update: I think I get it.
When you start a heavy process, its pid and tid are equal.
But then,  if it spawns threads,  their tids are  different from the pid  of the
heavy process.
You can check  this by looking at the  first line in the output  of the previous
`ps(1)` command: the first cmus process has a  pid equal to its tid, but not the
other threads.

---

In case  you wonder  why `htop(1)`  considers that  you have  *multiple* firefox
tasks, maybe it's because one task is created per tab you've opened and visited.
Or maybe viewing an embedded video on a webpage starts another task.
Or maybe downloading a file from a webpage starts another task.
...
You get the idea: the browser rarely performs only one task.

---

Note that not all programs are multithreaded.
For example, WeeChat doesn't seem to be.
Press `\  weechat Enter`  in htop,  then `H` to  show the  user threads:  no new
process is displayed.

OTOH, cmus is multithreaded.
Press `\ cmus Enter` in htop, then `H`  to show the user threads: a bunch of new
processes are displayed.

Btw, zathura and Nvim are also multithreaded, but not Vim nor ranger.

---

<https://en.wikipedia.org/wiki/Thread_(computing)>
Difference Between Process and Thread: <https://www.youtube.com/watch?v=O3EyzlZxx3g>
Intro to Processes & Threads: <https://www.youtube.com/watch?v=exbKr6fnoUw>
Process Management (Processes and Threads): <https://www.youtube.com/watch?v=OrM7nZcxXZU>

## Define what a system call (or syscall) is

<https://en.wikipedia.org/wiki/System_call>

###
## Youtube playlist

<https://www.youtube.com/watch?v=Eqjm11U0JQM&list=PLU2hZmcNdDQJN43RL6pGcX79fQVBFlobl&index=1>

### Process Display

<https://www.youtube.com/watch?v=Eqjm11U0JQM>

Most of the time, a  process can be in 5 states, which you  can find in the STAT
column in the output of `ps(1)`:

    ┌────────────────────┬─────────────────────────────────────────────────────────────────────┐
    │ process state code │ description                                                         │
    ├────────────────────┼─────────────────────────────────────────────────────────────────────┤
    │ R                  │ running or runnable (on run queue)                                  │
    ├────────────────────┼─────────────────────────────────────────────────────────────────────┤
    │ S                  │ interruptible sleep (waiting for an event to complete)              │
    ├────────────────────┼─────────────────────────────────────────────────────────────────────┤
    │ D                  │ uninterruptible sleep (usually IO)                                  │
    ├────────────────────┼─────────────────────────────────────────────────────────────────────┤
    │ T                  │ stopped by job control signal                                       │
    ├────────────────────┼─────────────────────────────────────────────────────────────────────┤
    │ Z                  │ defunct ("zombie") process, terminated but not reaped by its parent │
    └────────────────────┴─────────────────────────────────────────────────────────────────────┘

---

To get rid of a zombie, you may need to kill and restart its parent.

---

I stopped around the 4:40 min mark, when the speaker mentioned the current shell
session; before going further, we need to better understand what is a session.

Finish reading the "terminally confused" pdf.

##
## ?

<http://www.linusakesson.net/programming/tty/>

## ?

Understand the output of `free -h`:

                   total       used        free      shared  buff/cache   available
    Mem:           3,6G        1,8G        408M        262M        1,4G        1,2G
    Swap:          3,8G         50M        3,8G

`Shared` is an obsolete concept and `total` is easy to understand:

                   used        free    buff/cache   available
    Mem:           1,8G        408M          1,4G        1,2G
    Swap:           50M        3,8G

## ?

<https://unix.stackexchange.com/questions/138463/do-parentheses-really-put-the-command-in-a-subshell/138498#comment772229_138498>

Why does the manual say "The order of expansions is:

... parameter and  variable expansion, ..., and command substitution  (done in a
left-to-right fashion); word splitting; and filename expansion." Isn't this kind
of misleading?
I had interpreted the manual to mean $x would be expanded first as it has higher
precedence than command substitution.
But apparently this is not the case, as you correctly point out.

    x=1
    echo $(x=2; echo $x)

Which has the priority: variable expansion or command substitution?

## ?

When  we execute  a job  in a  subshell, it's  automatically re-parented  to the
session leader (upstart atm).

I think it's because, even though there's only one command, the subshell doesn't
`exec()` to start the job (i.e. no optimization).
Instead, it `fork()`, then `execve()`.
Once the job has been started in the sub-sub-shell, the sub-shell dies (why?).
So, the job becomes orphan and is re-parented to the session leader.

MWE:

    $ (sleep 100 &)
    $ pstree -lsp $(pidof sleep)
    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───sleep(8274)˜
                                                ^-------------------------^

---

How to start a job as a daemon?

Solution 1:

    $ (cmd &)

Solution 2:

    $ cmd &
    $ disown %

I think the first solution is more powerful.
Because the second one doesn't work if the job takes time to be started.

If the job contains several commands:

    $ ({ cmd1; cmd2 ;} &)
    $ pstree -lsp $(pidof sleep)
    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───bash(11880)───sleep(11881)˜
                                                                ^---------^
                                                                this time, the subshell doesn't die˜

---

Explain why none of these work:

    alias dropbox_restart='killall dropbox; ( "${HOME}/.dropbox-dist/dropboxd" ) &'
    alias dropbox_restart='killall dropbox; { "${HOME}/.dropbox-dist/dropboxd" & ;}'
    alias dropbox_restart='killall dropbox; { "${HOME}/.dropbox-dist/dropboxd" ;} &'

Note that according to [Gilles](https://unix.stackexchange.com/a/88235/289772):

   > Parentheses  create a  subshell whereas  braces  don't, but  this is  irrelevant
   > (except as a micro-optimization in some  shells) since a backgrounded command is
   > in a subshell anyway.

## ?

    $ cat /tmp/sh.sh
        #!/bin/bash
        /tmp/sh1.sh

    $ cat /tmp/sh1.sh
        #!/bin/bash
        sleep

    $ /tmp/sh.sh &

    $ pstree -lsp $(pidof sleep)
    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───tmux: server(2784)───zsh(29746)───sh.sh(32569)───sh1.sh(32+˜

If you kill `sh.sh`, you get this new process tree:

    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───sh1.sh(32570)───sleep(32571)

This shows  that when  a process  dies, its child  is re-parented  to init  or a
subreaper (here the session leader upstart).

If you kill `sh1.sh`, you get this new process tree:

    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───sleep(32571)

Again, the orphan (`sleep`) is re-apparented to the session leader.

---

However, if you kill the shell from which the script was started, then `sleep` is killed too.

    $ pstree -lsp $(pidof sleep)
    systemd(1)───lightdm(980)───lightdm(1086)───upstart(1096)───tmux: server(2784)───zsh(29746)───sh.sh(32569)───sh1.sh(32+˜
                                                                                     ^-^
    $ kill -1 29746
            │
            └ TERM is not enough

This shows that when you exit a shell, the latter sends SIGHUP to all its children.

---

    systemd(1)---lightdm(980)---lightdm(1086)---upstart(1096)---sh(6583)---run-or-raise(6584)---firefox(6586)...
                                                                │          │
                                                                │          └ /bin/bash ~/bin/run-or-raise firefox
                                                                │
                                                                └ sh -c ~/bin/run-or-raise firefox

    systemd(1)---lightdm(980)---lightdm(1086)---upstart(1096)---sh(2426)---run-or-raise(2427)---urxvt(2429)-+-urxvt(2430)
                                                                │          │
                                                                │          └ /bin/bash /home/user/bin/run-or-raise urxvt
                                                                │
                                                                └ sh -c ${HOME}/bin/run-or-raise urxvt

---

    $ cat /tmp/sh.sh
        mousepad [&]

    $ /tmp/sh.sh

        systemd(1)---lightdm(980)---lightdm(1086)---upstart(1096)---mousepad(30648)-+-{dconf worker}(30649)
                                                                                    |-{gdbus}(30651)
                                                                                    `-{gmain}(30650)

## Orphan process

A  process can  be orphaned  unintentionally, such  as when  the parent  process
terminates or crashes.
The  process group  mechanism can  be used  to help  protect against  accidental
orphaning, where in coordination with the user's shell will try to terminate all
the child processes with the "hangup"  signal (SIGHUP), rather than letting them
continue to run as orphans.
More precisely, as part of job control,  when the shell exits, because it is the
"session  leader" (its  session id  equals  its process  id), the  corresponding
login  session ends,  and  the shell  sends  SIGHUP to  all  its jobs  (internal
representation of process groups).

It is sometimes desirable to intentionally  orphan a process, usually to allow a
long-running job  to complete  without further  user attention,  or to  start an
indefinitely running service or agent.
Such  processes   (without  an  associated   session)  are  known   as  daemons,
particularly if they are indefinitely running.
A  low-level approach  is to  fork  twice, running  the desired  process in  the
grandchild, and immediately terminating the child.
The grandchild process  is now orphaned, and is not  adopted by its grandparent,
but rather by init.
In any event, the session id (process  id of the session leader, the shell) does
not change,  and the process id  of the session that  has ended is still  in use
until all orphaned processes either terminate  or change session id (by starting
a new session via setsid(2)).

To  simplify system  administration,  it is  often desirable  to  use a  service
wrapper so that processes not designed  to be used as services respond correctly
to system signals.
An alternative  to keep  processes running  without orphaning them  is to  use a
terminal multiplexer and  run the processes in a detached  session (or a session
that becomes detached), so the session is  not terminated and the process is not
orphaned.

A server process is also said to  be orphaned when the client that initiated the
request unexpectedly crashes  after making the request while  leaving the server
process running.

These  orphaned processes  waste server  resources and  can potentially  leave a
server starved for resources.
However, there are several solutions to the orphan process problem:

   - Extermination is the  most commonly used technique; in this  case the
     orphan is killed.

   - Reincarnation is  a technique in  which machines periodically try  to
     locate the parents  of any remote  computations; at which point  orphaned
     processes are killed.

   - Expiration is a technique where each process is allotted a certain amount
     of time to finish before being killed.  If need be a  process may "ask" for
     more time to  finish before the allotted time expires.

## Zombie process

A zombie  process or defunct process  is a process that  has completed execution
(via the exit system call) but still has  an entry in the process table: it is a
process in the "Terminated state".
This occurs for  child processes, where the  entry is still needed  to allow the
parent process to read its child's exit status: once the exit status is read via
the wait system call,  the zombie's entry is removed from  the process table and
it is said to be "reaped".
A child  process always  first becomes  a zombie before  being removed  from the
resource table.
In most cases,  under normal system operation zombies are  immediately waited on
by their parent and then reaped by  the system – processes that stay zombies for
a long time are generally an error and cause a resource leak.

The term zombie process derives from the common definition of zombie — an undead
person.
In  the term's  metaphor, the  child process  has "died"  but has  not yet  been
"reaped".
Also,  unlike normal  processes, the  kill  command has  no effect  on a  zombie
process.

Zombie processes should not be confused with orphan processes: an orphan process
is a process that is still executing, but whose parent has died.
These do not remain as zombie  processes; instead, (like all orphaned processes)
they are adopted by init (process ID 1), which waits on its children.
The result is that a process that is  both a zombie and an orphan will be reaped
automatically.

When a process ends via exit, all of the memory and resources associated with it
are deallocated so they can be used by other processes.
However, the process's entry in the process table remains.
The parent can read  the child's exit status by executing  the wait system call,
whereupon the zombie is removed.
The wait call may be executed in sequential code, but it is commonly executed in
a handler for the SIGCHLD signal, which the parent receives whenever a child has
died.

After  the zombie  is removed,  its process  identifier (PID)  and entry  in the
process table can then be reused.
However, if a parent fails to call wait,  the zombie will be left in the process
table, causing a resource leak.
In some situations this may be desirable – the parent process wishes to continue
holding this resource – for example  if the parent creates another child process
it ensures that it will not be allocated the same PID.

Zombies can be identified in the output from the Unix ps command by the presence
of a "Z" in the "STAT" column.
Zombies that exist for more than a short period of time typically indicate a bug
in the parent  program, or just an  uncommon decision to not  reap children (see
example).
If the parent program is no  longer running, zombie processes typically indicate
a bug in the operating system.
As with other resource leaks, the presence  of a few zombies is not worrisome in
itself, but may indicate a problem that would grow serious under heavier loads.
Since there is no memory allocated to  zombie processes – the only system memory
usage is  for the  process table entry  itself – the  primary concern  with many
zombies is not  running out of memory,  but rather running out  of process table
entries, concretely process ID numbers (`$ cat /proc/sys/kernel/pid_max`).

To remove zombies  from a system, the  SIGCHLD signal can be sent  to the parent
manually, using the kill command.
If the parent process still refuses to reap  the zombie, and if it would be fine
to terminate  the parent  process, the  next step  can be  to remove  the parent
process.
When a process loses its parent, init becomes its new parent.
init periodically executes the wait system call to reap any zombies with init as
parent.

---

<https://unix.stackexchange.com/a/5648/289772>

You may sometimes see entries marked Z in the ps or top output.
These  are technically  not  processes,  they are  zombie  processes, which  are
nothing more than an entry in the  process table, kept around so that the parent
process can be notified of the death of its child.
They will go away when the parent process pays attention via wait(2) (or dies).

---

How to reap a zombie?

    $ gdb -p PPID
    (gdb) call waitpid(PID, 0, 0)
    (gdb) quit

PID is the pid of the zombie, and PPID is the pid of its parent.

<https://serverfault.com/a/101525>

You can test this solution like so:

    $ cat <<'EOF' >/tmp/zombie.c
    // https://vitux.com/how-to-create-a-dummy-zombie-process-in-ubuntu/
    #include <stdlib.h>
    #include <sys/types.h>
    #include <unistd.h>
    int main ()
    {
    pid_t child_pid;child_pid = fork ();
    if (child_pid > 0) {
    // replace 3600 with the time in seconds during which the zombie should run
    sleep (3600);
    }
    else {
    exit (0);
    }
    return 0;
    }
    EOF

    $ cc /tmp/zombie.c -o /tmp/zombie
    $ /tmp/zombie &!
    $ ps ax -o pid,ppid,stat,args | grep defunct
    22511 22510 ZN   [zombie] <defunct>˜

    $ gdb -p 22510
    (gdb) call waitpid(22511, 0, 0)
    (gdb) quit
    $ ps ax -o pid,ppid,stat,args | grep defunct
    ''˜

    $ killall zombie

Document that there are at least two other ways:

   - kill the parent (should always work: the zombie is adopted by a subreaper, then reaped)
   - send SIGCHLD to the parent (may not work if the parent doesn't wait(2) – I think)

---

From `man 2 wait`:

   > A child that terminates, but has not been waited for becomes a "zombie".
   > The kernel maintains a minimal set of information about the zombie process (PID,
   > termination status, resource usage information) in  order to allow the parent to
   > later perform a wait to obtain information about the child.
   > As long as a zombie is not removed from the system via a wait, it will consume a
   > slot  in the  kernel process  table, and  if this  table fills,  it will  not be
   > possible to create further processes.
   > If a parent process terminates, then  its "zombie" children (if any) are adopted
   > by init(1), which automatically performs a wait to remove the zombies.

---

   > processes that stay zombies for a long time are generally an error and cause a **resource leak**.
<https://en.wikipedia.org/wiki/Zombie_process>

   > In  computer  science,  a  resource  leak  is  a  particular  type  of  resource
   > consumption by a  computer program where the program does  not release resources
   > it has acquired.
   > This condition is normally the result of a bug in a program.

   > Examples  of resources  available in  limited  numbers to  the operating  system
   > include  internet sockets,  file  handles, **process  table  entries, and  process**
   > **identifiers (PIDs)**.
   > Resource leaks  are often a  minor problem, causing  at most minor  slowdown and
   > being recovered from after processes terminate.
   > In  other  cases  resource  leaks  can be  a  major  problem,  causing  resource
   > starvation  and severe  system  slowdown or  instability,  crashing the  leaking
   > process, other processes, or even the system.
   > Resource leaks often go unnoticed under light load and short runtimes, and these
   > problems only manifest themselves under heavy system load or systems that remain
   > running for long periods of time.

<https://en.wikipedia.org/wiki/Resource_leak>

   > In computing,  a (system)  resource is  any physical  or virtual  component of
   > limited availability within a computer system.
   > Every device connected to a computer system is a resource.
   > Every internal system component is a resource.
   > Virtual  system  resources  include  files (concretely  file  handles),  network
   > connections (concretely network sockets), and memory areas.

<https://en.wikipedia.org/wiki/System_resource>

---

Document that `~/bin/signals-disposition`  is useful to check  whether a process
is blocking `CHLD`, which can explain why it has zombies.
And that  more generally, `/proc/PID/status` contains  a lot of useful  info for
debugging an issue.

I think that when a process blocks a signal, it tells the OS never to send it.
OTOH, when a process ignores a signal, the OS can still send it, but the process
doesn't react to it.

## Daemon

A daemon is a program that runs as a background process, rather than being under
the direct control of an interactive user.
Traditionally,  the  process names  of  a  daemon end  with  the  letter d,  for
clarification that  the process  is in  fact a  daemon, and  for differentiation
between a daemon and a normal computer program.
For example, syslogd is the daemon  that implements the system logging facility,
and sshd is a daemon that serves incoming SSH connections.

The parent process of a daemon is often, but not always, the init process.
A daemon is usually either created by a process forking a child process and then
immediately exiting,  thus causing init  to adopt the  child process, or  by the
init process directly launching the daemon.
In addition,  a daemon launched  by forking  and exiting typically  must perform
other operations, such as dissociating the process from any controlling terminal
(tty).
Such procedures  are often implemented  in various convenience routines  such as
daemon(3).

Systems often start daemons at boot time which will respond to network requests,
hardware activity, or other programs by performing some task.
Daemons such as cron may also perform defined tasks at scheduled times.

The term  is inspired from  Maxwell's demon, an  imaginary being from  a thought
experiment that constantly works in the background, sorting molecules.
Unix systems inherited this terminology.
Maxwell's Demon is consistent with  Greek mythology's interpretation of a daemon
as  a supernatural  being working  in the  background, with  no particular  bias
towards good or evil.

After  the  term  was  adopted  for  computer use,  it  was  rationalized  as  a
"backronym" for Disk And Execution MONitor.

Daemons which connect to a computer network are examples of network services.

In a  strictly technical sense,  a process is a  daemon when its  parent process
terminates and the daemon is assigned the init process as its parent process and
has no controlling terminal.
However, more generally a daemon may  be any background process, whether a child
of the init process or not.

The common method for a process to  become a daemon, when the process is started
from the  command-line  or from a  startup script  such as an  init script  or a
SystemStarter script, involves:

   - Optionally removing unnecessary variables from environment.

   - Executing as a background task by  forking and exiting (in the parent
     "half" of the fork).  This  allows daemon's  parent (shell  or  startup
     process)  to receive  exit notification and continue its normal execution.

   - Dissociating from  the controlling tty

   - Creating a new session  and becoming the session leader of that session.

   - Becoming a process group leader.
     These  three  steps  are  usually  accomplished  by  a  single  operation,
     setsid().

   - If  the daemon  wants  to  ensure that  it  won't  acquire a  new
     controlling  tty even  by  accident  (which happens  when  a session
     leader without a controlling tty opens a free tty), it may fork and exit
     again.  This means  that it is no  longer a session  leader in the new
     session, and can't acquire a controlling tty.

   - Setting the root directory (/) as  the current working directory so that
     the process does  not keep any directory  in use that  may be on
     a  mounted file system (allowing it to be unmounted).

   - Changing the umask to 0 to allow open(), creat(), and other operating
     system calls to provide their  own permission masks and not to  depend on
     the umask of the caller

   - Closing all inherited files  at the time of execution that are left open
     by the  parent process, including file descriptors 0,  1 and 2 for the
     standard streams (stdin, stdout and stderr).  Required files will be opened
     later.

   - Using a logfile, the console, or /dev/null as stdin, stdout, and stderr

If the process is  started by a super-server daemon, such  as inetd, launchd, or
systemd, the  super-server daemon will  perform those functions for  the process
(except for old-style  daemons not converted to run under  systemd and specified
as Type=forking and "multi-threaded" datagram servers under inetd).

