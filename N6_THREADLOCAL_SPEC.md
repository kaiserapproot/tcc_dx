# N6 thread_local / TLS runtime specification

Status: N6-00 SPEC FREEZE

この文書は、C++98サブセットにおける thread_local の初期対応範囲と、
後続フェーズで使用するTLSランタイムの所有関係を固定する。
N6-00ではコンパイラ、ランタイム、生成ヘッダ、既存のN5実装を変更しない。

## 1. 対応範囲

### 1.1 対応する構文

初期N6で対応するのは、namespace/global scopeにある定義だけとする。

    thread_local int value;
    thread_local int value = 123;
    thread_local MyClass object;

対象は、thread_local指定子を持つ完全なオブジェクト型の定義である。
初期対応では、関数内の宣言、宣言だけの参照、クラス内の静的データメンバは
対象に含めない。

### 1.2 未対応構文

次の構文は、通常のstatic/global objectとして処理してはならない。

    void f() {
        static thread_local MyClass x;
    }

    extern thread_local MyClass x;

    struct A {
        static thread_local MyClass x;
    };

    struct A {
        thread_local MyClass x;
    };

thread_localをstaticまたはexternと組み合わせた定義、クラスの非静的メンバ、
関数内定義、複数の宣言指定子を含む未定義の形式も同じ扱いとする。

未対応形式は、既存のstatic/global経路へフォールバックせず、
thread_local form is unsupported in N6 相当のコンパイルエラーでfail-closed
する。

### 1.3 Cモード

Cモードではthread_localを新しいC++キーワードとして扱わない。
既存のCソースにおける字句、識別子解決、TLS関連の既存処理は変更しない。

## 2. 型分類

N6の型分類と意味論は次の3分類で固定する。

| 分類 | storage | constructor | destructor |
|---|---|---|---|
| TLS_TRIVIAL | threadごと | 不要または単純初期化 | なし |
| TLS_NONTRIVIAL_CTOR | threadごと | 各threadの初回アクセスで1回 | なし |
| TLS_NONTRIVIAL_DTOR | threadごと | 各threadの初回アクセスで1回 | 構築したthreadの終了時に1回 |

TLS_TRIVIALはscalarまたはPOD相当を対象とし、初期値が指定された場合は
そのthreadのstorageに同じ初期値を持たせる。
TLS_NONTRIVIAL_CTORとTLS_NONTRIVIAL_DTORの初期対応は、既定構築可能な
クラス型に限定する。

配列、参照、関数型、未完成型、引数付きconstructorが必要な定義、
または生成された初期化・破棄経路の安全性を証明できない型は、
対応済みとして扱わずコンパイルエラーにする。
これらの型を追加する場合は、別の仕様変更とAuthority testを先に行う。

## 3. 初期化

### 3.1 lazy initialization

thread_local objectは、宣言が解析された時点や別threadのアクセス時点では
構築しない。各threadから最初にそのobjectへアクセスした時点で初期化する。

同じ宣言に対する状態は、threadごとに独立して次の状態機械を持つ。

    UNINITIALIZED
          |
          v
    INITIALIZING
          |
          v
    INITIALIZED

INITIALIZEDになった後の同じthreadからのアクセスではconstructorを再実行
しない。別threadは独自のUNINITIALIZED状態から開始する。

### 3.2 初期化失敗と再入

N6の現行C++98サブセットでは例外によるconstructor再試行を実装しない。
constructorが完了しなかったobjectはINITIALIZEDとして扱わず、未構築objectの
destructorも登録しない。

INITIALIZING中に同じthreadから同じobjectを再び初期化しようとする
recursive TLS initializationは未対応とする。未初期化値を返す、二重構築する、
または通常のglobal/static経路へ逃がすことは禁止し、診断または安全な失敗で
fail-closedする。

cleanup_startedが設定された後のthreadで、まだアクセスされていない
thread_local objectを初めて初期化することも未対応とする。
destructor実行中の新規TLS初期化と、新しいdestructor entryの登録は
fail-closedする。cleanup中にregistryへentryを追加して処理を継続してはならない。

## 4. storageと寿命のAuthority

### 4.1 per-thread storage

thread_local変数Vは、process全体で1個のstorageを持たない。

    V
     +-- thread A: storage A + initialized A
     +-- thread B: storage B + initialized B
     +-- thread C: storage C + initialized C

同じthread内ではVのaddressは安定し、異なるthread間ではaddressと値を
共有しない。thread Aで構築されたVをthread Bのstorageとして扱ってはならない。

### 4.2 destructor owner

N6のdestructor ownerはthreadである。

thread Aで構築されたobjectは、thread Aのcleanupでだけ破棄する。
process終了時までprocess-global listに残すこと、別threadのcleanupから破棄
すること、N5のprocess-wide destructor registryへ登録することを禁止する。

### 4.3 thread state

runtimeは、実行contextごとにthread単位のTCBを所有する。
概念上の状態は次のとおりとする。

    struct tcc_cpp_tls_thread_state {
        void *storage_context;
        struct tls_dtor_entry *dtor_entries;
        unsigned dtor_count;
        unsigned dtor_capacity;
        int cleanup_started;
        int cleanup_completed;
    };

destructor entryは、構築済みobjectのdestroy functionとobject addressを保持する。
上記は意味論を示すモデルであり、N6-01以降の内部構造をこのC構造体へ固定する
ものではない。

## 5. destructor registry

### 5.1 独立registry

N5とN6のregistryは共有しない。

    N5: process-wide static/local-static lifetime
    N6: per-thread thread_local lifetime

N6のentryをN5の終了登録へ混ぜない。N5のprocess-wide終了処理を、N6の
thread-local objectの所有者やcleanup起点として再利用しない。

N6 runtime APIは、少なくとも次の責務を独立して持つ。

    tcc_cpp_tls_get_storage(...)
    tcc_cpp_tls_register_dtor(...)
    tcc_cpp_tls_thread_cleanup(...)

APIの引数、TLS variable keyの表現、storage allocationの詳細はN6-01の実装
設計で確定する。ただし、storage取得、destructor登録、thread cleanupが
N5から独立したruntime責務であることはこの文書で確定する。

### 5.2 登録と破棄の順序

destructorはconstructorが正常完了した後にだけ登録する。
破棄は、そのthreadで実際に構築が完了した順序の逆順で行う。

    touch(c);
    touch(a);
    touch(b);

の場合、同じthreadの終了時は次の順序になる。

    construct: C A B
    destroy:  B A C

宣言順、symbol table順、process-wide登録順をAuthorityにしない。

### 5.3 exactly-once

cleanup開始時にcleanup_startedを設定し、cleanup_completed後の再呼出しは
何もしない。cleanup_startedが1かつcleanup_completedが0の途中でcleanupへ
再入した場合も、registryを再走査せず、二度目のcleanup passを開始しない。
同じobjectについて、次を同時に満たす。

    初回アクセスされたthreadのconstructor = ちょうど1回
    未アクセスthreadのconstructor = 0回
    destructor = 実際に構築されたthreadでちょうど1回

未構築objectのdestructor、二重destructor、他threadのobjectのdestructorは
すべて不正とする。

## 6. Windows backend

Win32とWin64は、同じN6 runtime abstractionを使用する。

    generated code
          |
          v
    N6 TLS runtime abstraction
          |
          v
    Windows TLS/FLS backend

TCB identityとstorageのAuthorityは、runtimeが所有するWindows OS TLS slotと
そのslotから得るper-thread TCBとする。thread-exit cleanupを起動するnotification
mechanismはN6-03で確定し、N6-00ではFLS callbackをthread終了のAuthorityとして
凍結しない。

compiler frontendやgenerated/user codeがTlsAlloc、FlsAlloc、TlsGetValue、
FlsGetValueなどのOS APIを直接呼び出してはならない。
TLS key、TCB、per-thread storage map、destructor registryの所有者はruntime
とする。

FLS callbackを補助的に使用する余地は残すが、FLS callback単独をOS thread-exit
Authorityとして使用してはならない。fiber deletion、FLS index release、fiber
switchをOS thread terminationとして扱わず、FLS callbackの引数だけでTCB identity、
storage、objectの構築状態を決めない。

Win32/x86とWin64/x64で同じsemantic APIを使用し、backend内部だけがABIや
pointer sizeの差を吸収する。対応外CPUでは、x86/x64用の生成コードを流用せず
fail-closedする。

## 7. 実行形態

| 経路 | N6 policy |
|---|---|
| normal EXE | SUPPORTED |
| main return | SUPPORTED |
| exit() | SUPPORTED_MAIN_THREAD_ONLY |
| worker threadの正常終了 | SUPPORTED |
| tcc -run | SUPPORTED |
| libtccのtcc_run()経路 | SUPPORTED |
| libtccのtcc_relocate()後の任意host呼出し | LIMITED / 明示cleanupが必要 |
| DLL | NOT_SUPPORTED_IN_N6 |

libtccは全面対応とはせず、tcc_run()が管理する実行contextを基準にする。
tcc_relocate()後にhostが任意のthreadを作成し、runtime contextの寿命を越えて
codeまたはFLS cleanup callbackを実行する形はN6対象外とする。

DLLではthread_local定義を通常のglobal/staticへフォールバックしない。
sourceからの直接生成とobjectを経由したfinal linkの両方で、
DLL thread_local is unsupported in N6 相当の明示エラーにする。

## 8. cleanupの合流

### 8.1 worker thread

worker threadが対応する通常のthread終了経路へ到達したとき、そのthreadの
TCBをcleanupする。cleanup対象はそのthreadのentryだけである。

### 8.2 main thread

main returnとexit()は、同じN6 runtime cleanupへ合流する。
どちらの経路でもmain threadの構築済みTLS objectを一度だけ破棄する。

normal EXE、tcc -run、libtccのtcc_run()は、終了処理の実装が異なっても
次のsemantic APIへ合流する。

    tcc_cpp_tls_thread_cleanup()

main thread cleanup完了後に、同じcontextでN6 cleanupを再実行してはならない。
main returnとexit()の完全対応は、N6に参加したworker threadがすべて終了し、
join済みであることを前提とする。exit()の対応範囲はmain threadからの呼出し
だけとし、worker threadからのexit()はfail-closedとする。workerが生きたまま
processが終了する場合はN6としてunsupportedであり、そのworkerのTLS destructor
実行を保証しない。

### 8.3 対象外の異常終了

TerminateThread、TerminateProcess、_exit相当のprocess terminationなど、
通常のthread cleanupを通らない異常終了についてはdestructor実行を保証しない。
この制限を隠して通常終了と同じ結果を報告してはならない。

また、tcc -runまたはlibtccのexecution contextを破棄する前にworker threadを
終了・joinすることをruntime lifetimeの前提とする。JIT code上のcleanup callback
がcontext破棄後に呼ばれる状態は許可しない。

## 9. N6フェーズ境界

### N6-01: TLS storage primitive

    thread_local int value;

だけを対象に、per-thread address isolationとvalue isolationを実装する。
constructor、destructor、LIFO registryはまだ実装しない。

### N6-02: per-thread initialization

既定構築可能なnon-trivial classを対象に、threadごとの初回アクセスconstructor
とINITIALIZED guardを実装する。

### N6-03/N6-04: destructor registry and LIFO

threadごとのdtor entry登録、thread終了時cleanup、構築順逆順の破棄を実装する。

### N6-05: main-thread termination

main returnとexit()を共通cleanupへ接続し、exactly-onceを検証する。

### N6-06: tcc -run / libtcc

通常EXEと同じsemantic cleanup APIを、tcc_run()のruntimeへ接続する。
直接tcc_relocate()利用の範囲は、この文書のLIMITED policyを越えて拡張しない。

### N6-07: fail-closed

未対応構文、DLL、未対応型、recursive initializationをsilent fallbackなしで
拒否する。

### N6-08: regression

既存C経路、N5 process-wide destructor、非TLS C++ global/local staticの挙動を
変更しないことを確認する。

## 10. N6-00 SPEC FREEZE AUTHORITY

    N6_SCOPE_NAMESPACE_THREAD_LOCAL=YES
    N6_TRIVIAL_TLS_SUPPORTED=YES
    N6_NONTRIVIAL_CTOR_SUPPORTED=YES
    N6_NONTRIVIAL_DTOR_SUPPORTED=YES
    N6_FUNCTION_STATIC_THREAD_LOCAL=NO
    N6_EXTERN_THREAD_LOCAL=NO
    N6_CLASS_STATIC_THREAD_LOCAL=NO
    N6_TLS_INITIALIZATION=PER_THREAD_LAZY_ON_FIRST_ACCESS
    N6_TLS_STORAGE=PER_THREAD
    N6_TLS_ADDRESS=PER_THREAD
    N6_CTOR_ON_FIRST_ACCESS_EXACTLY_ONCE=YES
    N6_UNACCESSED_THREAD_CTOR_COUNT=0
    N6_DTOR_PER_CONSTRUCTED_THREAD_EXACTLY_ONCE=YES
    N6_DTOR_ORDER=REVERSE_CONSTRUCTION_ORDER_PER_THREAD
    N6_DTOR_REGISTRY=PER_THREAD
    N6_N5_DESTRUCTOR_REGISTRY_SHARED=NO
    N6_NORMAL_EXE=SUPPORTED
    N6_MAIN_RETURN=SUPPORTED
    N6_EXIT=SUPPORTED_MAIN_THREAD_ONLY
    N6_WORKER_THREAD_EXIT=SUPPORTED
    N6_TCC_RUN=SUPPORTED
    N6_LIBTCC=LIMITED
    N6_DLL=NOT_SUPPORTED
    N6_WINDOWS_X86=SUPPORTED
    N6_WINDOWS_X64=SUPPORTED
    N6_WINDOWS_TCB_STORAGE=OS_THREAD_TLS
    N6_FLS_AS_TCB_AUTHORITY=NO
    N6_FLS_CALLBACK_AS_THREAD_EXIT_AUTHORITY=NO
    N6_THREAD_EXIT_HOOK=N6_03_TO_BE_PROVEN
    N6_EXIT_CALLER=MAIN_THREAD
    N6_PROCESS_EXIT_REQUIRES_WORKERS_JOINED=YES
    N6_EXIT_FROM_WORKER=FAIL_CLOSED
    N6_EXIT_WITH_LIVE_N6_WORKERS=UNSUPPORTED
    N6_CLEANUP_REENTRANCY=NO_REENTER
    N6_CLEANUP_ALREADY_STARTED=NO_SECOND_DTOR_PASS
    N6_TLS_FIRST_INITIALIZATION_DURING_CLEANUP=FAIL_CLOSED
    N6_DTOR_REGISTRATION_AFTER_CLEANUP_STARTED=FAIL_CLOSED
    N6_UNSUPPORTED_FORM_POLICY=FAIL_CLOSED
    N6_C_SOURCE_BEHAVIOR_CHANGE=NONE
    N6_EXISTING_C_TLS_PATH_CHANGE=NONE
    N6_N5_BEHAVIOR_CHANGE=NONE
    N6_00_CODE_IMPLEMENTATION=NONE
    N6_00_SPEC_FREEZE=PASS
