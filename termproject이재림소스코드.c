// 2021313628 이재림 - RR scheduling
// 50 시간양자
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// 프로세스 정보를 담는 구조체
struct pI {
    int mKey;       // 메시지 키
    int cBT;        // CPU 버스트 시간
    int ioBT;       // I/O 버스트 시간
    int rcBT;       // 남은 CPU 버스트 시간
    int rioBT;      // 남은 I/O 버스트 시간
    int ioStart;    // I/O 시작 시간
};

// 메시지 버퍼 구조체
struct msgbuf {
    long msgtype;   // 메시지 타입
    char msg[64];   // 메시지
};

int tflag = 1;
struct pI upI();
void pIsnd();
void printcpI();
struct pI cpI[10];  // 프로세스 정보를 담는 배열
int rQ[10] = {0, }; // 실행 큐
int wQ[10] = {0, }; // 대기 큐
int cnt = 0;        // 타이머 틱 수
pid_t pids[10];     // 프로세스 ID를 담는 배열
FILE *fp;

void timer_handler(int signum) {
    static int rQnum = 10;  // 실행 큐 길이
    int comp = 0;           // 완료된 프로세스 수
    int mKey = rQ[0];       // 실행 큐의 첫 번째 프로세스의 메시지 키

    key_t key_id;           // 메시지 큐 식별자
    struct msgbuf mbuf;     // 메시지 버퍼

    key_id = msgget((key_t)2020, IPC_CREAT | 0666);  // 메시지 큐 생성 또는 접근
    memset(&mbuf, 0, sizeof(mbuf));
    char s[10] = "A";

    strncpy(mbuf.msg, s, sizeof(mbuf.msg) - 1);

    // 대기 큐에 있는 프로세스에게 메시지 전송 및 I/O 시간 감소
    for (int i = 0; i < 10; i++) {
        if (wQ[i] > 0) {
            mbuf.msgtype = wQ[i];
            msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);
            cpI[(wQ[i] / 10) - 1].rioBT -= 50;
        }
        if (cpI[(wQ[i] / 10) - 1].rioBT == 0)
            wQ[i] = 0;
    }

    // 실행 중인 프로세스에게 메시지 전송 및 CPU 시간 감소
    mbuf.msgtype = mKey;
    strncpy(mbuf.msg, s, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);
    cpI[(mKey / 10) - 1].rcBT -= 50;

    // 프로세스의 CPU 버스트 완료시 실행 큐와 대기 큐 갱신
    if (cpI[(mKey / 10) - 1].rcBT == 0) {
        for (int i = 0; i < rQnum; i++)
            rQ[i] = rQ[i + 1];
        rQ[rQnum - 1] = 0;
        rQnum -= 1;
        wQ[(mKey / 10) - 1] = mKey;
    } else {
        int tmp = rQ[0];
        for (int i = 0; i < rQnum; i++)
            rQ[i] = rQ[i + 1];
        rQ[rQnum - 1] = tmp;
    }

    // 완료된 프로세스 개수 카운트
    for (int i = 0; i < 10; i++) {
        if ((cpI[i].rcBT == 0) && (cpI[i].rioBT == 0))
            comp++;
    }
    cnt++;

    // 프로세스 정보 출력
    if (cnt <= 2000 && cnt >= 0) {
        printcpI();

        // 응답 시간 및 실행 시간 계산
        int completed_processes = 0;
        int total_response_time = 0;
        int total_execution_time = cnt * 50; // 시간 양자가 50인 경우

        for (int i = 0; i < 10; i++) {
            if (cpI[i].cBT - cpI[i].rcBT > 0) { // 실행된 프로세스일 때
                total_response_time += (cpI[i].cBT - cpI[i].rcBT);
                completed_processes++;
            }
        }

        float average_response_time = (float)(cpI[0].cBT - cpI[0].rcBT);
        fp = fopen("schedule_dump.txt", "a");

        fprintf(fp, "Average Response Time: %.2f ms\n", average_response_time);
        fprintf(fp, "Total Execution Time: %d ms\n", total_execution_time);
      // 파일 닫기
      fclose(fp);
    } else {
       fp = fopen("schedule_dump.txt", "a");

      fprintf(fp, "Finished\n");
        exit(0);
    }

    // 모든 프로세스 완료 여부 확인
    if (comp == 10)
        tflag = 0;
}

void pIsnd(int mKey[], int cBT[], int ioBT[], int rcBT[], int rioBT[]) {
    key_t key_id;               // 메시지 큐 식별자
    struct msgbuf mbuf;         // 메시지 버퍼

    key_id = msgget((key_t)2020, IPC_CREAT | 0666);  // 메시지 큐 생성 또는 접근
    memset(&mbuf, 0, sizeof(mbuf));

    char s1[10], s2[10], s3[10], s4[10], s5[10];  // 문자열 배열

    // 메시지 전송을 위해 데이터를 문자열로 변환하고 메시지 큐에 전송
    mbuf.msgtype = 1;
    sprintf(s1, "%d", *mKey);
    strncpy(mbuf.msg, s1, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);

    mbuf.msgtype = 2;
    sprintf(s2, "%d", *cBT);
    strncpy(mbuf.msg, s2, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);

    mbuf.msgtype = 3;
    sprintf(s3, "%d", *ioBT);
    strncpy(mbuf.msg, s3, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);

    mbuf.msgtype = 4;
    sprintf(s4, "%d", *rcBT);
    strncpy(mbuf.msg, s4, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);

    mbuf.msgtype = 5;
    sprintf(s5, "%d", *rioBT);
    strncpy(mbuf.msg, s5, sizeof(mbuf.msg) - 1);
    msgsnd(key_id, (void *)&mbuf, sizeof(mbuf.msg), IPC_NOWAIT);
}

struct pI upI() {
    key_t key_id;               // 메시지 큐 식별자
    struct msgbuf mbuf;         // 메시지 버퍼

    key_id = msgget((key_t)2020, IPC_CREAT | 0666);  // 메시지 큐 생성 또는 접근
    memset(&mbuf, 0, sizeof(mbuf));

    char s1[10], s2[10], s3[10], s4[10], s5[10];  // 문자열 배열

    // 메시지 큐로부터 데이터 수신 및 문자열로 변환하여 프로세스 정보 업데이트
    msgrcv(key_id, (void *)&mbuf, sizeof(mbuf.msg), 1, 0);
    strcpy(s1, mbuf.msg);
    msgrcv(key_id, (void *)&mbuf, sizeof(mbuf.msg), 2, 0);
    strcpy(s2, mbuf.msg);
    msgrcv(key_id, (void *)&mbuf, sizeof(mbuf.msg), 3, 0);
    strcpy(s3, mbuf.msg);
    msgrcv(key_id, (void *)&mbuf, sizeof(mbuf.msg), 4, 0);
    strcpy(s4, mbuf.msg);
    msgrcv(key_id, (void *)&mbuf, sizeof(mbuf.msg), 5, 0);
    strcpy(s5, mbuf.msg);

    char *tmp;
    struct pI cp;

    // 문자열로 받은 데이터를 정수형으로 변환하여 프로세스 정보 업데이트
    tmp = s1;
    cp.mKey = atoi(tmp);
    tmp = s2;
    cp.cBT = atoi(tmp);
    tmp = s3;
    cp.ioBT = atoi(tmp);
    tmp = s4;
    cp.rcBT = atoi(tmp);
    tmp = s5;
    cp.rioBT = atoi(tmp);

    return cp;  // 업데이트된 프로세스 정보 반환
}

void printcpI() {
  // 파일 열기 (이어쓰기 모드)
  fp = fopen("schedule_dump.txt", "a");

    // 현재 프로세스 상태 출력
      fprintf(fp,"\n\n\n\n\t***************************** Tic = %d ************************************\n", cnt);
      fprintf(fp,"\n\n\n\t\t\t\t\tRun Queue\n\n");
      fprintf(fp,"\t\tProcess ID\t\t남은 CPU Burst Time(ms)\t\t실행된 CPU Burst Time\n");
      fprintf(fp,"\t----------------------------------------------------------------------------\n");
    for (int i = 0; i < 10; i++) {
        if (cpI[i].rcBT > 0) {
              fprintf(fp,"\t\t%d\t\t\t\t\t%d\t\t\t\t\t%d\n", pids[i], cpI[i].rcBT, (cpI[i].cBT - cpI[i].rcBT));
        }
    }
    fprintf(fp,"\n\n\n\n\t\t\t\t\tWait Queue\n\n");
    fprintf(fp,"\t\tProcess ID\t\t남은 I/O Burst Time(ms)\t\t실행된 I/O Burst Time\n");
    fprintf(fp,"\t----------------------------------------------------------------------------\n");
    for (int i = 0; i < 10; i++) {
        if ((cpI[i].rcBT == 0) && (cpI[i].rioBT > 0)) {
            fprintf(fp,"\t\t%d\t\t\t\t\t%d\t\t\t\t\t%d\n", pids[i], cpI[i].rioBT, (cpI[i].ioBT - cpI[i].rioBT));
        }
    }
      fprintf(fp,"\n\n\n\t****************************************************************************\n");
  // 파일 닫기
  fclose(fp);
}

int main(void){
    pid_t cpid[10];                // 자식 프로세스의 PID 배열
    int pNum = 0;
    struct pI cp[10];              // 프로세스 정보를 담는 구조체 배열

    key_t key_id;
    struct msgbuf mbuf;

    key_id = msgget((key_t)2020, IPC_CREAT | 0666);  // 메시지 큐 생성 또는 접근
    memset(&mbuf, 0, sizeof(mbuf));

    // 10개의 자식 프로세스 생성
    for (pNum = 0; pNum < 10; pNum++) {
        cpid[pNum] = fork();  // 자식 프로세스 생성
        if (cpid[pNum] == 0) {  // 자식 프로세스인 경우
            srand(time(NULL) ^ (getpid() << 16));  // 난수 생성을 위한 시드 설정

            // 프로세스 정보 초기화 및 메시지 전송
            cp[pNum].mKey = (pNum + 1) * 10;
            cp[pNum].cBT = ((rand() % 50) * 100) + 5000;
            cp[pNum].ioBT = ((rand() % 50) * 100) + 5000;
            cp[pNum].rcBT = cp[pNum].cBT;
            cp[pNum].rioBT = cp[pNum].ioBT;
            cp[pNum].ioStart = rand() % cp[pNum].cBT; // 무작위 I/O 시작 시간 설정

            // 프로세스 정보를 메시지로 전송
            int *mKey, *cBT, *ioBT, *rcBT, *rioBT;
            mKey = &cp[pNum].mKey;
            cBT = &cp[pNum].cBT;
            ioBT = &cp[pNum].ioBT;
            rcBT = &cp[pNum].rcBT;
            rioBT = &cp[pNum].rioBT;

            pIsnd(mKey, cBT, ioBT, rcBT, rioBT);

            // 프로세스의 실행 시간과 I/O 대기열 관리
            while ((cp[pNum].rcBT + cp[pNum].rioBT) > 0) {
                if (cp[pNum].rcBT > 0) {
                    cp[pNum].rcBT -= 50;
                } else {
                    if (cnt == cp[pNum].ioStart) {
                        // I/O 시작 시간에 도달하면 I/O 대기열로 진입
                        int idx = 0;
                        while (wQ[idx] != 0) {
                            idx++;
                        }
                        wQ[idx] = cp[pNum].mKey;
                        cp[pNum].rioBT -= 50;
                        break;
                    } else {
                        cp[pNum].rioBT -= 50;
                    }
                }
            }
            exit(0);  // 자식 프로세스 종료
        } else {
            cpI[pNum] = upI();  // 부모 프로세스가 자식 프로세스 정보 업데이트
            rQ[pNum] = cpI[pNum].mKey;
            pids[pNum] = cpid[pNum];
        }
    }

    struct sigaction sa;
    struct itimerval timer;

    // 타이머 관련 설정
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 50000;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 50000;
    printcpI();

    setitimer(ITIMER_VIRTUAL, &timer, NULL);  // 가상 타이머 시작
    while (tflag);  // 타이머 종료 대기

    wait(NULL);  // 모든 자식 프로세스가 종료되기를 기다림

    while (1) {
        // 모든 프로세스가 완료되었는지 확인
        int all_completed = 1;
        for (int i = 0; i < 10; ++i) {
            if (cpI[i].rcBT > 0 || cpI[i].rioBT > 0) {
                all_completed = 0;
                break;
            }
        }

        if (all_completed) {
            // 대기열에 프로세스가 없는지 확인하고 완전히 종료되면 루프 종료
            int no_process_in_queue = 1;
            for (int i = 0; i < 10; ++i) {
                if (rQ[i] != 0 || wQ[i] != 0) {
                    no_process_in_queue = 0;
                    break;
                }
            }

            if (no_process_in_queue) {
                break;
            }
        }

        cnt++;  // 카운터 증가
        printcpI();  // 현재 프로세스 상태 출력
        sleep(1); // 혹은 원하는 딜레이 설정
    }

    return 0;
}