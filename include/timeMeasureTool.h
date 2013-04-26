/**
 * @file    timeMeasureTool.h
 * @brief   検証用ソケット通信機能
 *
 * @author  TTDC
 * @date    2012-08-07 新規作成
 *
 * Copyright (c) 2012 TOYOTA MOTOR CORPORATION.
 * (Insert a Licence Condition Here.)
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include    <stdio.h>           /* C言語標準ヘッダ類            */
#include    <stdlib.h>
#include    <unistd.h>
#include    <stdbool.h>
#include    <string.h>
#include    <time.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/mman.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <signal.h>
#include    <errno.h>
#include    <pthread.h>
#include    <ctype.h>

/* ビルドコンフィグレーション          */
#define VEHICLEINFO_DAEMON_COEXIST  1   /* マスターとスレーブを同一CPUに置く場合には１にする */
    /* この場合、共有メモリやライブラリとの通信ポートを別にする */

#define VEHICLEINFO_DAEMON_INET 1       /* Linux内 Daemon〜Library間通信のドメイン    */
    /*   0＝UNIXドメイン(AF_UNIX)を使用する       */
    /*   1＝INETドメイン(AF_INET)を使用する       */
    /*   UNIXドメインの方が性能的には有利だが、    */
    /*   SMACK色々不自由な点も多いので、INETを使う。 */

/* コンフィグレーション           */
#define WEBSOCKET6          0   /* WebSocketnのプロトコルバージョン6対応   */
    /*  (厳密な試験はしていないので、使う場合は  */
    /*   試験実施のこと)                     */

/* 各種定義値                  */
#define DEFAULT_HOST        "127.0.0.1" /* 既定のマスタのホスト名(localhost) */
#define DEFAULT_PORT        39200       /* 既定のポート番号ベース                */
#define DEFAULT_PORT_WB     39201       /* 既定のポート番号ベース(WebSocket用)  */
#define DEFAULT_PORT_NT     39202       /* 既定のポート番号ベース(Native用)     */
#define DEFAULT_PORT_NT2    39203       /* 既定のポート番号ベース(Native用)     */
#define VEHICLEINFO_MAX_LOG_LINES   5000        /* ログのファイル当たりの最大行数        */
#define MEASURE_MAX_LOG_FILES   10      /* ログの最大個数                        */

#define CONNECT_TIMER 10        /* ソケットConnect待ち時間(秒) */
#define CONNECT_WAIT        (200)       /* マスタとの接続リトライ時間(ミリ秒単位) */
#define CLIENT_BIND_WAIT    (180*1000)  /* Bind失敗時の最大待ち時間(ミリ秒単位)      */
#define TRANSLOG_NUM        (240)       /* 転送ログ蓄積個数                         */

#define DAEMON_MASTER       (0) /* 自身がマスターデーモン                    */
#define DAEMON_SLAVE        (1) /* 自身がスレーブデーモン                    */
#define DAEMON_NAME         "timeMeasureRecv"   /*自身のデーモン名                  */

/* システム制限値                */
#define VEHICLEINFO_MAX_PRIV            8       /* 読み込み/書き込み権限の最大値      */
#define VEHICLEINFO_MAX_CLIENT          50      /* 最大クライアント(Library)数         */
#define VEHICLEINFO_WEBSOCKET_MAXPROT   128     /* WebSocketのプロトコル処理の最大電文長(CR,LFを含む) */
#define TEST_WEBSOCKET_MAXPROT  1024*1024       /* WebSocketのプロトコル処理の最大電文長(CR,LFを含む) */
#define MEASURE_SOCKET_MAXDATA  2048    /* Socketの最大電文長(CR,LFを含む) */

/* ファイルディレクトリ           */
#define MEASURE_LOG_DIR     "/var/log/vehicleinfo/log"
    /* ログ出力先ディレクトリ                */

/* 接続状態                         */
#define CONNSTAT_UNKNOWN          0     /* 不明                                     */
#define CONNSTAT_DATA      1    /* WebSocket,データ転送中                 */
#define CONNSTAT_DATA_NT   3    /* NativeのSocket,データ転送中               */
#define CONNSTAT_HS0       10   /* WebSocket,ハンドシェイク中(GET未受信)    */
#define CONNSTAT_HS1       11   /* WebSocket,ハンドシェイク中(キー未受信)   */
#define CONNSTAT_HS2       12   /* WebSocket,ハンドシェイク中(キー受信済)   */
#if     WEBSOCKET6              /* WebSocket旧プロトコル(バージョン6)対応   */
#define CONNSTAT_HS11     21    /* WebSocket旧プロトコル、Key1受信Key2未受信 */
#define CONNSTAT_HS12     22    /* WebSocket旧プロトコル、Key2受信Key1未受信 */
#define CONNSTAT_HS13     23    /* WebSocket旧プロトコル、Key1,2受信済、空行待ち */
#define CONNSTAT_HS14     24    /* WebSocket旧プロトコル、Key1,2受信済、ボディ待ち */
#define CONNSTAT_DATA6    2     /* WebSocket,データ転送中(旧プロトコル)     */
#endif  /*WEBSOCKET6*/                        /* WebSocket旧プロトコル(バージョン6)対応   */

/* 受信データ保持バッファ                */
    typedef struct T_MEASURE_SAVEDATA_t
    {
        unsigned short Size;    /* 保持しているバイト数           */
        unsigned short BufSize; /* バッファの大きさ(バイト数)   */
        char *Buf;              /* 保持データ(動的アロケーション) */
    } S_MEASURE_SAVEDATA_t;

/* クライアント管理テーブル         */
    typedef struct T_MEASURE_MNG_t
    {
        int Socket;             /* ソケット(ファイルディスクリプタ)         */
        short ConnStat;         /* 接続状態                                 */
        char Res[2];            /* (空き)                                 */
        S_MEASURE_SAVEDATA_t SaveData;  /* 受信データ保持バッファ                   */
#if     WEBSOCKET6              /* WebSocket旧プロトコル(バージョン6)対応   */
        int Key1;               /* WebSocketのKey1                          */
        int Key2;               /* WebSocketのKey2                          */
#endif  /*WEBSOCKET6*/                        /* WebSocket旧プロトコル(バージョン6)対応   */
    } S_MEASURE_MNG_t;

/* 転送ログ                 */
#define TRANSLOG_INFO_START     0x0201  /* 情報ログ(処理開始)                       */
#define TRANSLOG_INFO_END       0x0202  /* 情報ログ(処理終了)                       */
#define TRANSLOG_INFO_DOWN      0x0203  /* 情報ログ(異常終了)                       */
#define CLILOG_SEND_WSDT        0x1021  /* クライアントへの送信ログ(WebSocketデータ) */
#define CLILOG_RECEIVE_WSDT     0x1022  /* クライアントからの受信ログ(WebSocketデータ) */
#define CLILOG_RECEIVE_NTDT     0x1023  /* クライアントからの受信ログ(Nativeデータ) */
#if     WEBSOCKET6              /* WebSocket旧プロトコル(バージョン6)対応   */
#define CLILOG_SEND_WSDT6       0x1021  /* クライアントへの送信ログ(WebSocketデータ) */
#define CLILOG_RECEIVE_WSDT6    0x1022  /* クライアントからの受信ログ(WebSocketデータ) */
#define CLILOG_RECEIVE_NTDT6    0x1023  /* クライアントからの受信ログ(Nativeデータ) */
#endif  /*WEBSOCKET6*/                        /* WebSocket旧プロトコル(バージョン6)対応   */
#define CLILOG_SEND_WSHS        0x1031  /* クライアントへの送信ログ(WebSocketハンドシェイク) */
#define CLILOG_RECEIVE_WSHS     0x1032  /* クライアントからの受信ログ(WebSocketハンドシェイク) */
#define CLILOG_CONNECT          0x1101  /* クライアントとの接続ログ                 */
#define CLILOG_DISCONNECT       0x1102  /* クライアントとの切断ログ                 */

#define MEASURELOG_MAX_STR    1024      /* ログの文字列の最大サイズ                 */
#define MEASURE_CLILOG_MAX_STR  (sizeof(struct timeval) + MEASURELOG_MAX_STR)

    typedef struct T_MEASURELOG_t
    {
        short LogType;          /* ログの種別                               */
        short Id;               /* 車両情報ID(クライアントならクライアントID) */
        short Size;             /* データサイズ                             */
        short Info;             /* 付加情報                                 */
        struct timeval Time;    /* 送受信時刻(自マシンの日付時刻)           */
        struct timeval DataTime;        /* データ更新日付時刻(自マシンの日付時刻)   */
        union
        {
            short Svalue;
            int Ivalue;
            long long Lvalue;
            float Fvalue;
            double Dvalue;
            char Cvalue[MEASURELOG_MAX_STR + 1];
            /* 文字列は256文字                            */
        } d;                    /* 送受信データ                               */
    } S_MEASURELOG_t;

// スレッド起動時引数
    typedef struct T_NTRECV_t
    {
        int Socket;             // ソケット番号
        int nRcvBufSize;        // 受信バッファサイズ
        FILE *LogFd;            // ログファイルポインタ
        int client_idx;         // クライアント番号
        pthread_mutex_t printLock;      // ログ出力時ミューテクス
    } S_NTRECV_t;

/* 内部関数のプロトタイプ宣言(RECV)            */
    void DisconnectClient(int ClientID);
    /* クライアントとの回線切断処理               */
    int ClientHandShake(int ClientID);  /* クライアントからのハンドシェイク受信       */
    int ClientRequest(int ClientID);    /* クライアントからの要求受信              */
    void *NativeRecieve(void *);        /* Nativeソケット受信関数 */


/* 内部関数のプロトタイプ宣言(SEND)            */
    void TEST_SOCKET_CLOSE();
    char *TEST_GET_TIME();
    double TEST_GET_TIME2();
    char *TEST_GET_LOGMSG(char *msg);
    char *TEST_GET_LOGMSG2(char *msg);
    char *TEST_GET_TOP();       //未使用
    char *TEST_GET_PID(char *pProcName);
    char *ltrim(char *s);
    char *rtrim(char *s);
    char *trim(char *s);
    char *strim(char *base, char *rmvstr);
    int TEST_SOCKET_CREATE_THREAD(int port, char pname[]);
    void *TEST_SOCKET_RUN(void *arg);
    int TEST_SOCKET_PREPARE_SEND(char msg[]);

#ifdef __cplusplus
}
#endif

/**
 * End of File. (timeMeasureTool.h)
 */
