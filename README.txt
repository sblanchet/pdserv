PROCESS DATA COMMUNICATIONS Tools

Suppose that you have written a funky process control program. You have lots
of input signals, either as a result of sampling field signals or due to
calculation. Furthermore, these calculations are influenced by parameters that
you would like to change from time to time.

These types of programs typically run in real time. A real time process is
severly limited in the system resources it is allowed to use. Practically all
system calls involve memory transfers of some sort. New memory space has to be
allocated, which means that the calling process will be blocked for a short
while.

For desktop users, this is hardly noticeable, except when swap space is being
used. For real time applications, this is a show stopper!

So how can you interact with the process without using system calls? How can
you see what your signals are doing without using some sort of network
communication?

This library allows you to do just that without using system calls in the
context of your application! Of coarse it uses system calls, otherwise it
would be rather introverted. These system calls are done in a separate
communications process that is forked off during initialization. Any blocking
calls to system fuctions will thus not influence you original application.

The two processes, i.e. your application and the communications process, 
communicates with each other exclusively via shared memory that was
obtained by using anonymous mmap (see the manpage) during initialisation.

So what prevents this shared memory from being swapped out? YOU, if you wish!
A real time application usually calls mlock() somewhere during initialization
to tell the kernel not to swap out memory to disk. The library does NOT do
this automatically. First of all, only root is allowed to call mlock().
Secondly, mlock is practically called by every real time process somewhere. If
this is not required, the library will not force you!

This library is not limited to real time processes (in which case you would not
use mlock() - see above). The only requirement is that an update() function is
called every time a calculation result is completed.

The library assumes various concepts that need explanation:
*) Application
        The whole program is a single application, i.e. one and only one
        process in the system's process table. An application consists of one
        or more tasks

*) Tasks
        A task represents a single thread of execution and consists of one or
        more signals that are calculated every time the task executes.  Every
        time a task completes an execution, the library update() must be
        called, allowing the communications process to copy the newly
        calculated signals to the communications task.  For example, you may
        have a slow running task at 10Hz, but for a particular loop you have
        another running at 1kHz.

        A task usually implemented as a separate thread running in a
        while(true) loop paced by a timer. Therefore, tasks have a sample time
        property specifying how often the signals are calculated in a second.
        It is the responsibility of your application to ensure that the task's
        timing.  However, the library does not rely on exact timing.

*) Signals
        A signal belongs to a single task.  Signals can have any of the basic
        data types up to 64bit (8 bytes) and have any dimension.  Its value is
        updated during every run of the task. Complex data types, such as
        structures or complex numbers, are not allowed.

        Signals are the results of calculations and can only be viewed by the
        user. However, parameters can be used to influence the calculation
        results, allowing indirect influence on the signals.

        Signals are registered with the library during initialization and
        exist in the space of the task. They are visible via a path property
        that is specified during registration.

*) Parameters
        Parameters are similar to signals, but are a property of the application
        rather than tasks. The same properties and restrictions apply as far
        as the data type and dimensions are concerned.

        Parameters are requested from the library during initialization. The
        application only gets a constant pointer to the parameter, thus
        preventing accidental modification by the application.

