Troubleshooting fxall_gateway core dumps.

	lscsa63757:cores% gdb fxall_gateway core.fxall_gateway.27059.1412322776
	
First enable C++ name demangling:

	(gdb) set print asm-demangle on


	(gdb) bt
	#0  0xffffe410 in __kernel_vsyscall ()
	#1  0x001fcdf0 in raise () from /lib/libc.so.6
	#2  0x001fe701 in abort () from /lib/libc.so.6
	#3  0x0630d54b in os::abort(bool) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#4  0x063bd851 in VMError::report_and_die() () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#5  0x06312469 in JVM_handle_linux_signal () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#6  0x0630f458 in signalHandler(int, siginfo*, void*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#7  <signal handler called>
	#8  0xf7e6250d in std::deque<std::string, std::allocator<std::string> >::operator=(std::deque<std::string, std::allocator<std::string> > const&) () from /panther/apps/panther.phase-11.1/lib/MLCommon.so
	#9  0xf7e601b7 in ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&) () from /panther/apps/panther.phase-11.1/lib/MLCommon.so
	#10 0x081e1360 in ClientManager::getPantherGroup(long long const&) ()
	#11 0x08169cf6 in FXAllRequest::FXAllRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&) ()
	#12 0x081a488f in FXAllSwapQuoteRequest::FXAllSwapQuoteRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&) ()
	#13 0x08341966 in TCPIBridge::OnQuoteRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&) ()
	#14 0x08343cc8 in Java_TCPIBridge_OnQuoteRequest ()
	#15 0x9ade8241 in ?? ()
	#16 0x9ade7cf4 in ?? ()
	#17 0x9adb93d4 in ?? ()
	#18 0x9ab033b9 in ?? ()
	#19 0x9ab00249 in ?? ()
	#20 0x0621c40d in JavaCalls::call_helper(JavaValue*, methodHandle*, JavaCallArguments*, Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#21 0x06310378 in os::os_exception_wrapper(void (*)(JavaValue*, methodHandle*, JavaCallArguments*, Thread*), JavaValue*, methodHandle*, JavaCallArguments*, Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#22 0x0621bd20 in JavaCalls::call_virtual(JavaValue*, KlassHandle, symbolHandle, symbolHandle, JavaCallArguments*, Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#23 0x0621bdad in JavaCalls::call_virtual(JavaValue*, Handle, KlassHandle, symbolHandle, symbolHandle, Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#24 0x0628bfb5 in thread_entry(JavaThread*, Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#25 0x06391bbd in JavaThread::run() () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#26 0x06311029 in java_start(Thread*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
	#27 0x00321832 in start_thread () from /lib/libpthread.so.0
	#28 0x002a5e0e in clone () from /lib/libc.so.6

Check signal info at signalHandler

(gdb) frame 6
#6  0x0630f458 in signalHandler(int, siginfo*, void*) () from /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so
(gdb) x/16wx $ebp
0x748fe5e8:     0x748febdc      0xffffe600      0x0000000b      0x748fe5fc
0x748fe5f8:     0x748fe67c      0x0000000b      0x00000000      0x00000080
0x748fe608:     0x00000000      0x00000000      0x9f400034      0x00000000
0x748fe618:     0x9f400040      0x00000050      0x0023cee6      0x0816650f
(gdb) x/8wx $ebp
0x748fe5e8:     0x748febdc      0xffffe600      0x0000000b      0x748fe5fc
0x748fe5f8:     0x748fe67c      0x0000000b      0x00000000      0x00000080

The signal is 0x0000000b (SIGSEGV). Segfault when call std::deque<string> assignment.

	(gdb) frame 9
	#9  0xf7e601b7 in ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&) () from /panther/apps/panther.phase-11.1/lib/MLCommon.so

	(gdb) i r
	eax            0x44bfa118       1153409304
	ecx            0x44bfa320       1153409824
	edx            0x0      0
	ebx            0x748febd0       1955589072
	esp            0x748fea28       0x748fea28
	ebp            0x748feb30       0x748feb30
	esi            0x748fea64       1955588708
	edi            0xf7f30c5c       -135066532
	eip            0xf7e601b7       0xf7e601b7 <ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&)+567>
	eflags         0x200296 [ PF AF SF IF ID ]
	cs             0x23     35
	ss             0x2b     43
	ds             0x2b     43
	es             0x2b     43
	fs             0x0      0
	gs             0x63     99

	(gdb) x/8wx $ebp
	0x748feb30:     0x748fec70      0x081e1360      0x748febd0      0x083c7dd8
	0x748feb40:     0x083c9dc0      0x748feba8      0x44bf9c00      0x44bf1258

- 0x748febd0  this pointer
- 0x083c7dd8  argument 1 (char const*), this gives us where the exception is thrown
- 0x083c9dc0  argument 2 (char const*)
- 0x748feba8  argument 3 (ml::common::diagnostics::StackTrace const&), a reference. reference is implemented as a pointer, so it's actual location in memory is 0x44be1690

	(gdb) x/s 0x083c7dd8
	0x83c7dd8:      "exes/fx/fxall_gateway/services/ClientManager.cpp (line 97)"
	(gdb) x/s 0x083c9dc0
	0x83c9dc0 <ClientManager::getPantherGroup(long long const&)::__PRETTY_FUNCTION__>:      "ml::spec::panther::identity::PantherGroup::Ptr ClientManager::getPantherGroup(const int64&)"
	(gdb) x/wx 0x748feba8
	0x748feba8:     0x44be1690

The location of the code is ml::common::exception::Exception::prepareMessage+567. Let's locate it in source code. The original c++ code is:

	/** ----------------------------------------------------------------------------------
	 */
	void Exception::prepareMessage(const char *location, const char *fsig, const ml::common::diagnostics::StackTrace & st)
	{
		std::ostringstream oss;
		oss << ml::common::util::Path::baseName(location)
			<< " : caught " 
			<< which() 
			<< " in [ " 
			<< fsig 
			<< " ]";
		
		if( m_strWhy.size() ) {
			oss << " : " 
				<< m_strWhy;
		}
		
		oss << "\nStackTrace:\n" << st;
		m_stackTrace = st;
		m_strWhat = oss.str();
	}	

We need to look at assembly code:

	% cd /export/work/zk8xmsb/panther/banafix-rtf/build/heap/objs/MLCommon/libs/common/exception
	% objdump -S -d -w Exception.o | c++filt > Exception.s
	% vim Exception.s


The function starts at 0x1070, the location is 0x1070 + 567 = 4208 + 567 = 4775 = 0x12a7
Due to inlining, we see a lot of code:


	/** ----------------------------------------------------------------------------------
	 */
	void Exception::prepareMessage(const char *location, const char *fsig, const ml::common::diagnostics::StackTrace & st)
	{
	    1070:   55                      push   %ebp
	    1071:   89 e5                   mov    %esp,%ebp
	    1073:   57                      push   %edi
	    1074:   56                      push   %esi
	       *  The default constructor does nothing and is not normally
	       *  accessible to users.
	      */
	    
	    ....

	    oss << "\nStackTrace:\n" << st;

	    1290:   ff 75 14                pushl  0x14(%ebp)
	    1293:   56                      push   %esi
	    1294:   e8 fc ff ff ff          call   1295 <ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&)+0x225>
	    1299:   58                      pop    %eax
	    129a:   8d 43 0c                lea    0xc(%ebx),%eax
	    129d:   5a                      pop    %edx
	    129e:   ff 75 14                pushl  0x14(%ebp)
	    12a1:   50                      push   %eax
	    12a2:   e8 fc ff ff ff          call   12a3 <ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&)+0x233>
	    12a7:   8b 85 4c ff ff ff       mov    -0xb4(%ebp),%eax
	    12ad:   83 c4 10                add    $0x10,%esp

	    ...

	#if _GLIBCXX_FULLY_DYNAMIC_STRING == 0
	      if (__builtin_expect(this != &_S_empty_rep(), false))
	    1431:   81 fa 00 00 00 00       cmp    $0x0,%edx
	    1437:   0f 85 03 02 00 00       jne    1640 <ml::common::exception::Exception::prepareMessage(char const*, char const*, ml::common::diagnostics::StackTrace const&)+0x5d0>
	      // PR 58265, this should be noexcept.
	      basic_string&
	      operator=(basic_string&& __str)
	      {
	    // NB: DR 1204.
	    this->swap(__str);
	    143d:   83 ec 08                sub    $0x8,%esp

	    m_stackTrace = st;
	    m_strWhat = oss.str();

	    1440:   83 c3 04                add    $0x4,%ebx
	    1443:   57                      push   %edi


We can see that the problem is this line:

	    m_stackTrace = st;

The declaration of StackTrace confirms that it has a deque<string>:

	class StackTrace
	{
	public:
		StackTrace(bool performTrace = true);
	    std::string toString() const;
	    
	private:
		friend class mlc::application::Application;
		static void staticInitialize();
		static void staticDestruct();

	private:
	    std::deque< std::string > m_frames;
	};

To find out more about the cause of the core, we can simply scan for strings around the deque

	(gdb) frame 8
	#8  0xf7e6250d in std::deque<std::string, std::allocator<std::string> >::operator=(std::deque<std::string, std::allocator<std::string> > const&) () from /panther/apps/panther.phase-11.1/lib/MLCommon.so

	(gdb) x/8wx $ebp
	0x748febdc:     0x44be19f0      0x00000008      0x44bfa118      0x44bfa118
	0x748febec:     0x44bfa318      0x44be19fc      0x44bfa118      0x44bfa118

	(gdb) x/32s 0x44bfa118
	0x44bfa118:     ""
	0x44bfa119:     "\301\344\257p&\a\257order: makerId='MLFX' dealDate='20141003' dealTime='07:52:56' fxAllId='60424082' inCompetition=true instrument='FX.CROSS' makerGroupName='MLFX' makerInstitution='MLFX' makerLocation='NY' makerN"...
	0x44bfa1e1:     "ame='MLFX' makerOrderConfirmation=[Null] makerOrderCustom=[Null] makerOrderReference=[Null] nonNegotiated=false mbeOrder=false orderBatchCount=0 orderBatchId=0 originalMakerId=[Null] originalFxAllId='"...
	0x44bfa2a9:     "' originalTakerId=[Null] primeBrokerTaker=[Null] protocol=CLASSIC_RFQ serverMatched=false state=LIVE takerGroupName\r\002"
	0x44bfa31f:     ""
	0x44bfa320:     "d\353aE\\k\263DTi8G\224\272\262D\304k\263D\324i8G\274\031\276D\334\031\276D\374\272\262D\034\273\262DdL<GL\332;Gt)\263D\234\212\070G\004\005\276D\254y\346G\274%\317E\024&\317E\244\332;GConfirmation=[Null] takerOrderCustom=[Null] takerOrderReference=[Null] takerId='abelmejdoub' truncationOrder=false fee=[Null"...
	0x44bfa3e8:     "] feeCurrency=[Null] autoTradable=true instrumentOrders={ 0=[ FXAllCfxOrder: orderType=SWAP legs={ 0=[ FXAllCfxInstrumentLeg: valueDateType='TODAY' valueDate='20141003' takerBuysBase=[Null] spotDate='"...
	0x44bfa4b0:     "20141007' dealtCurrency='DKK' dealtAmount=40000000.0 contraAmount=[Null] riskCurrency=[Null] riskAmount=[Null] midRa\261\n"
	0x44bfa527:     ""
	0x44bfa528:     "@"
	0x44bfa52a:     "@\237@"
	0x44bfa52e:     "@\237"
	0x44bfa531:     ""
	0x44bfa532:     ""
	0x44bfa533:     ""
	0x44bfa534:     ""
	0x44bfa535:     ""
	0x44bfa536:     ""
	0x44bfa537:     ""
	0x44bfa538:     "ntManager.cpp (line 97) : caught ml::common::exception::ConfigurationException in [ ml::spec::panther::identity::PantherGroup::Ptr ClientManager::getPantherGroup(const int64&) ] : Unable to retrieve i"...
	0x44bfa600:     "nvalid group: 10350216\nStackTrace:\n          ClientManager::getPantherGroup(long long const&)\n          FXAllRequest::FXAllRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&)\n         "...
	0x44bfa6c8:     " FXAllSwapQuoteRequest::FXAllSwapQuoteRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&)\n  tC1E\030\002"
	0x44bfa73b:     ""
	0x44bfa73c:     "\204\004"
	0x44bfa73f:     ""
	0x44bfa740:     "m\004"
	0x44bfa743:     ""
	0x44bfa744:     "m\004"
	0x44bfa747:     ""
	0x44bfa748:     "\377\377\377\377          ClientManager::getPantherGroup(long long const&)\n          FXAllRequest::FXAllRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&)\n          FXAllSwapQuoteRequest::FXAllSw"...
	0x44bfa810:     "apQuoteRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&)\n          TCPIBridge::OnQuoteRequest(boost::shared_ptr<ml::spec::fx::fxAll::FXAllCorder> const&)\n          Java_TCPIBridge_On"...
	0x44bfa8d8:     "QuoteRequest\n          [0x9ade8241]\n          [0x9ade7cf4]\n          [0x9adb93d4]\n          [0x9ab033b9]\n          [0x9ab00249]\n          /panther/apps/jdk1.6.0_04/jre/lib/i386/client/libjvm.so [0x621"...
