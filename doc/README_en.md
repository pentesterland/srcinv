--[ Contents
1 - Introduction
  1.1 - The framework
  1.2 - The dependencies
  1.3 - The target
2 - Usage of this framework
3 - Collect information of the target project
4 - Parse the information we collected
  4.1 - ADJUST
  4.2 - GET_BASE
  4.3 - GET_DETAIL
  4.4 - GET_XREFS
  4.5 - GET_INDCFG1
  4.6 - GET_INDCFG2
5 - Use the information to do something
6 - Future work
7 - References
8 - Greetings



--[ 1 - Introduction
This article will show you how to use this framework, how this works, and the
target this framework could run on.



--[ 1.1 - The framework
SRCINV is short for source code investigation. It shows how I audit the source code.

Directories:
	analysis	self-defined structures, handle the collected, used by hacking
	bin		the compiled binaries
	config		runtime configuration
	collect		handle the original input, like a binary or a source tree
	core		main routine and common libs used in analysis and hacking
	doc		Documatations
	hacking		static analysis or fuzzing
	include		header files
	tmp		temporary folder, for collected output, etc.

Source code(or binary code) is not only for human who read it, but also for the
compiler(or interpreter or CPU). When compiling a source file, GCC will handle
several data formats, AST / GIMPLE / ... This framework collects the lower GIMPLEs,
researchers could use these information to test the project.
Normally, a GCC plugin could only see the information about the current compiling
file. When we put the information of all compiled files together, we could see
the big picture of the project, and more convenience to test the project.

This framework aims to do code audit automatically for open source projects. For
QA/researchers, they could find different kinds of bugs in their projects.
This framework may also automatically generate samples to test the bug and patches
for the bug.

For each project, this framework use a src structure to track all the index
information, which may be:
sibuf, a list for compiled files
resfile, a list for all the resfiles for the target project
sinodes, nodes for non-local variables, functions, data types. These nodes could be
	searched by name or by location. For static file-scope variables, static
	file-scope functions, data types have a location, we use location to search
	and insert. For global variables, global functions, and data types with a
	name but without a location, we use name to search and insert. For data types
	without location or name, we put them into sibuf.



--[ 1.2 - The dependencies
The framework could only run on 64bit GNU/Linux systems, and need personality
system call to disable the current process aslr.

The colorful prompt could be incompatible.

Libraries:
	clib (https://github.com/snorez/clib)
	ncurses
	readline
	libcapstone

Header files:
	gcc plugin library



--[ 1.3 - The target
For now, the framework is only for C projects compiled by GCC.



--[ 2 - Usage of this framework
This framework has some builtin commands:
	SRCINV> help
	========= USAGE INFO =========
	help:
				Show this help message
	exit:
				Exit this main process
	quit:
				Exit this main process
	load_plugin:
		(plugin_path) [plugin_args...]
	unload_plugin:
		(id|path)	Unload specific plugin
	reload_plugin:
		(id|path) [plugin_args]	Reload target plugin
	list_plugin:
				Show current loaded plugins
	do_make:
		(c|cpp|...) (sodir) (projectdir) (outfile) [extras]
				Build target project, make sure Makefile has EXTRA_CFLAGS
	do_sh:
				Execute bash command
	set_plugin_dir:
		(plugin_dir)	Set the plugin directory, the original still count
	showlog:
		Show current log messages
	load_srcfile:
		[srcfile]
	set_srcfile:
		(srcfile_name)
	getinfo:
		(res_path) (is_builtin) (linux_kernel?) (step)
			Get information of resfile, for step
				0 Get all information
				1 Get base information
				2 Get detail information
				3 Get xrefs information
				4 Get indirect call information
				5 Check if all GIMPLE_CALL are set
	staticchk:
		Run registered static check methods
	itersn:
		[output_path]	Traversal all sinodes to stderr/file
	sn_load:
		[id]		most for test cases
	========= USAGE END =========

	SRCINV> list_plugin
	0	0	loaded		fuzz
	>>>> /home/test/workspace/todo/srcinv/plugins/fuzz.so
	1	0	loaded		sibuf
	>>>> /home/test/workspace/todo/srcinv/plugins/sibuf.so
	2	1	loaded		staticchk
	>>>> /home/test/workspace/todo/srcinv/plugins/staticchk.so
	3	0	unload		(null)
	>>>> /home/test/workspace/todo/srcinv/plugins/test.so
	4	1	loaded		sinode
	>>>> /home/test/workspace/todo/srcinv/plugins/sinode.so
	5	7	loaded		getinfo
	>>>> /home/test/workspace/todo/srcinv/plugins/getinfo.so
	6	1	loaded		debuild
	>>>> /home/test/workspace/todo/srcinv/plugins/debuild.so
	7	0	loaded		sn_load
	>>>> /home/test/workspace/todo/srcinv/plugins/sn_load.so
	8	0	loaded		uninit
	>>>> /home/test/workspace/todo/srcinv/plugins/uninit.so
	9	1	loaded		resfile
	>>>> /home/test/workspace/todo/srcinv/plugins/resfile.so
	10	4	loaded		src
	>>>> /home/test/workspace/todo/srcinv/plugins/src.so
	11	1	loaded		gen_sample
	>>>> /home/test/workspace/todo/srcinv/plugins/gensample.so
	12	0	loaded		c
	>>>> /home/test/workspace/todo/srcinv/plugins/c.so
	13	0	loaded		itersn
	>>>> /home/test/workspace/todo/srcinv/plugins/itersn.so
	14	1	loaded		utils
	>>>> /home/test/workspace/todo/srcinv/plugins/utils.so

For more information(https://github.com/hardenedlinux/srcinv/tree/master/doc/commands.md).
This framework take a few steps to test the project: collect the information,
parse the information, use the information.



--[ 3 - Collect information of the target project
NOTE: before collecting information of a project, make sure the resfile not exist yet.

For a single file, you could use gcc -fplugin=, -fplugin-arg to compile it. For a
project which could use make, make sure the Makefile has EXTRA_CFLAGS options, or
some other options like that, then you can use the following commands to compile
the project:
make EXTRA_CFLAGS+='-fplugin=/.../x.so -fplugin-arg-x-output=/.../...'

To collect the information, for gcc, we could use GCC plugins.
For C projects, collect/c.cc does this job. It is independent from the main process.
It is used when compile the project.

About GCC plugin, this framework gets the lower GIMPLEs.
	the source file		--->	AST
	AST			--->	High-level GIMPLE
	High-level GIMPLE	--->	Low-level GIMPLE
We collect the GIMPLEs before cfg pass. Some passes are may obsolete in newest GCC.
	all_lowering_passes
	|--->	useless		[GIMPLE_PASS]
	|--->	mudflap1	[GIMPLE_PASS]
	|--->	omplower	[GIMPLE_PASS]
	|--->	lower		[GIMPLE_PASS]
	|--->	ehopt		[GIMPLE_PASS]
	|--->	eh		[GIMPLE_PASS]
	|--->	cfg		[GIMPLE_PASS]
	...
	all_ipa_passes
	|--->	visibility	[SIMPLE_IPA_PASS]
	...
For each function with a body, all passes in all_lowering_passes will be executed.
Which means some functions may not be lowered when we collect the current handling
function.
For more information about GCC plugin, check out refs[1] or the source code of GCC.

The information collect part is implemented in collect/, for c projects, it is c.cc.
It is a GCC plugin. When this plugin gets involved, it detects current compling
file's name and full path, register a pass manager before cfg pass, and a callback
function for PLUGIN_ALL_IPA_PASSES_START event which write the information to resfile.
The resfile is PAGE_SIZE aligned.

We can't register PLUGIN_PRE_GENERICIZE to handle each tree_function_decl, cause
the function may be incompleted.
	static void test_func0(void);
	static void test_func1(void)
	{
		test_func0();
	}

	static void test_func0(void)
	{
		/* test_func0 body */
	}
In this case, PLUGIN_PRE_GENERICIZE event happens, and the function test_func1 is
handled. However, the tree_function_decl for test_func0 is not initialized yet.

When the pass manager we registered gets called, it takes an object which is
struct function, and the decl field is the tree_function_decl. In all_lowering_passes,
the function body would be moved to tree_function_decl->f->gimple_body from
tree_function_decl->saved_tree.
When collect info of a function, if its saved_tree is not NULL, that means this
function still not lowered, we ignore it.
When collect info of a var, we focus on non-local variables. GCC provides a function
to test if this is a global var.
	static inline bool is_global_var (const_tree t)
	{
		return (TREE_STATIC(t) || DECL_EXTERNAL(t));
	}
for VAR_DECL, TREE_STATIC check whether this var has a static storage or not.
DECL_EXTERNAL check if this var is defined elsewhere.
We add an extra check:
	if (is_global_var(node) &&
			((!DECL_CONTEXT(node)) ||
			 (TREE_CODE(DECL_CONTEXT(node)) == TRANSLATION_UNIT_DECL))) {
			objs[start].is_global_var = 1;
		}
DECL_CONTEXT is NULL or the TREE_CODE of it is TRANSLATION_UNIT_DECL, we take this
var as a non-local variable.

Additionally, we need track every pointers, locations.

Some tests using collect/c.cc on Linux Kernel 4.14.x, make vmlinux -j9, take about
20 minutes, the resfile is about 19.4G.



--[ 4 - Parse the information we collected
The main feature of the framework is parsing the information. However, it is
the most complicated part. In this part, we should adjust the information we get,
index the information as srcfile which is used later.

This is implemented in plugin/getinfo.c, it detect the first several bytes of
the compiled file's information(enum si_lang_type), search the list to see if a
proper lang_ops is already registered.

For a better experience, the framework use ncurses to show the process status.
(need to make the framework with ver=release).

To parse the information, we need to load the resfile into memory. However, the
resfile for Linux Kernel 4.14.x is almost 20G. We can not load it all. On the other
hand, the resfile is also needed when use the information. Here comes the solution:
	Disable aslr and restart the process.

	Use mmap to load resfile at RESFILE_BUF_START, if the resfile current loaded
	in memory is larger than RESFILE_BUF_SIZE, unmap the last, and do the next
	mmap just at the end of the last memory area.

	For each compiled file, we use a sibuf to track the address where to load
	the information, the size, and the offset of the resfile.

	Use resfile__resfile_load to load a compiled file's information.

	memory layout:
	0x0			--- 0x400000		NULL pages
	0x400000		--- 0x403000		si_core .text
	0x602000		--- 0x603000		si_core .rodata ...
	0x603000		--- 0x605000		si_core .data ...
	0x605000		--- 0x647000		heap
	SRC_BUF_START		--- RESFILE_BUF_START	srcfile
	RESFILE_BUF_START	--- 0x????????		resfiles
	0x700000000000		--- 0x7fffffffffff	threads libs plugins stack...

If SRC_BUF_START is 0x100000000, RESFILE_BUF_START is 0x1000000000, the size of
srcfile is up to 64G, the size of resfile could be 1024G or more.



--[ 4.1 - ADJUST
First half of PHASE1. The information we collected, a lot of pointers in it.
We must adjust these pointers before we read it. And locations as well.
Note that, location_t in GCC is 4 bytes, so we set it an offset value.
use *(expanded_location *)(sibuf->payload + loc_value) to get the location.



--[ 4.2 - GET_BASE
The buttom half of PHASE1. Base info include:
	TYPE_FUNC_GLOBAL, TYPE_FUNC_STATIC
	TYPE_VAR_GLOBAL, TYPE_VAR_STATIC
	TYPE_TYPE_LOC, TYPE_TYPE_NAME
	TYPE_NONE(for data types, put into sibuf->type_nodes)

Check if the location or name exists in sinodes. If not, alloc a new sinode.
If exists, and searched by name, should check if the name conflict(weak symbols).



--[ 4.3 - GET_DETAIL
PHASE2, get detail of every sinodes.

For data types, need to know the data type it points to, or the size, or the fields.
For var, just get the type of it.
For functions, get the return value type, the arguments list, and the function body(
as code_path).

About function body, we take GIMPLE_LABEL as a seperator, the statement after a
GIMPLE_LABEL is the first statement of this code_path, the end of this code_path
could be a GIMPLE_SWITCH/GIMPLE_GOTO/GIMPLE_COND/GIMPLE_ASM(nl nonzero). In the
meantime, it detects the statements which are not reachable.
	static int test_func(void)
	{
		int err = 0;
		if (err)
			return 1;
		else
			return 0;
		return 0;		/* not reachable */
	}



--[ 4.4 - GET_XREFS
PHASE3, set possible_list for every non-local variables, get variables and
functions(not direct call) marked(use_at_list), get direct call.
A direct call is the second op of GIMPLE_CALL statement is addr of a function.
If the second op of GIMPLE_CALL is VAR_DECL or PARM_DECL, it is an indirect call.

The resfile of Linux Kernel 4.14.x could not get through the step yet.



--[ 4.5 - GET_INDCFG1
PHASE4, handle marked functions, if the statement is GIMPLE_ASSIGN, get the lhs of
this statement, then the var_node of lhs, add a possible_list.



--[ 4.6 - GET_INDCFG2
PHASE5, handle the indirect calls.

The second op of GIMPLE_CALL is VAR_DECL:
	get the target var_node of this VAR_DECL
	check the possible_list, add the functions into callee
	check the use_at_list, find out where this value get assigned.

For examples:
	static void test_func0(void);
	static void test_func1(void)
	{
		void (*testf)(void);
		testf = test_func0;
		testf();
	}
Or:
	static void test_func0(void);
	struct test_a {
		int a;
		void (*b)(void);
	};
	static struct test_a static_a = {
		.a = 1,
		.b = test_func0,
	};
	static void test_func1(void)
	{
		static_a.b();
	}
we could get test_func1 called by test_func0.

PARM_DECL is not handled for now.



--[ 5 - Use the information to do something
We need to write some plugins to use these information. As we collect the lower
GIMPLEs, these are what we handle in plugins.

For example:
	static void test_func(int flag)
	{
		int need_free;
		char *buf;
		if (flag) {
			buf = (char *)malloc(0x10);
			need_free = 1;
		}

		/* do something here */

		if (need_free)
			free(buf);
	}
plugins/uninit.cc shows how to detect this kind of bugs.
This plugin detects all functions one by one:
	generate all possible code_path of this function
	traversal all local variables(not static)
		traversal code_paths, find first position this variable used
		check if the first used statement is to read this variable
(The demo)(https://www.youtube.com/watch?v=anNoHjrYqVc)



--[ 6 - Future work
For Linux Kernel:
	get through PHASE3 PHASE4 PHASE5 without any errors
	get symbols in .s/.S files, to build the full call chains
	add mitigations detection
	...
Trace variables, where it comes from.
Identify the actions, read or write, or get the address of it.
Generate code_pathes for two different functions.
The dependencies of code_pathes. For example, sys_read need the result of sys_open.
Generate samples for a specified code_path.
Generate patches for some kind of bugs.
More program language support.
...



--[ 7 - References
  [0] this project homepage
  https://github.com/hardenedlinux/srcinv/

  [1] GNU Compiler Collection Internals
  https://gcc.gnu.org/onlinedocs/gcc-6.4.0/gccint.pdf

  [2] GCC source code
  http://mirrors.concertpass.com/gcc/releases/gcc-6.4.0/gcc-6.4.0.tar.gz

  [3] 深入分析GCC
  https://www.amazon.cn/dp/B06XCPZFKD

  [4] gcc plugins for linux kernel
  https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/scripts/gcc-plugins?h=v4.14.85



--[ 8 - Greetings
Thanks to the author of <<深入分析GCC>> for helping me to understand the inside of
GCC.

Thanks to PYQ and other workmates, with your understanding and support, I can focus
on this framework.

Thanks to CG for his support during the development of the framework and the write-up
of this article.

Certainly...
Thanks to all the people who managed to read the whole text.

There is still a lot of work to be done. Ideas or contributions are always welcome.
Feel free to send push requests.



---------[ EOF
