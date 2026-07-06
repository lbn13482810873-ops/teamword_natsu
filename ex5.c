#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#ifndef BANK_SIM_H
#define BANK_SIM_H

#define MAX_QUEUE 10
#define TOTAL_WINDOWS 10 //假设10个窗口

#define START_TIME (8 * 60)   // 480分钟
#define STOP_ARRIVE (17 * 60) // 1020分钟
#define CLOSE_TIME (20 * 60)  // 1200分钟

// 客户结构体
typedef struct {
    int id;
    int arrive_time;    // 到达时刻（从0点开始的分钟数）
    int duration;       // 业务耗时
    int start_serve;    // 开始办理时刻
    int wait_time;      // 等待时长
} Client;

// 窗口队列结构体
typedef struct {
    Client data[MAX_QUEUE];
    int front;
    int rear;
    int count;          // 当前排队人数
    int total_served;   // 该窗口全天接待总数
    int max_len;        // 该窗口出现过的最大队列长度
} Window;

int total_wait_time = 0;//办完的人总等待时间
int total_served_all = 0;//办完总人数

static int cmp_client_arrive(const void *a, const void *b){
    const Client *ca = (const Client *)a;
    const Client *cb = (const Client *)b;
    return ca->arrive_time - cb->arrive_time;
}


// --- 同学 A 负责的函数原型 ---
void generate_clients(Client clients[], int *total_clients);

// --- 同学 B 负责的函数原型 ---

//底层
//初始化窗口队列
void initWindow(Window *w){
    w->front = 0;
    w->rear = 0;
    w->count = 0;
    w->total_served = 0;
    w->max_len = 0;
}
//获取某个窗口当前排队人数
int GetQueueLength(Window *w){
    return w->count;
}
//客户入列
int enqueue(Window *w, Client c){
    if(w==NULL){
        return 0;
    }
    if(w->count >= MAX_QUEUE){
        return 0;
    } 
    w->data[w->rear] = c;
    w->rear = (w->rear + 1) % MAX_QUEUE;
    w->count++;
    if(w->count > w->max_len){
        w->max_len = w->count;
    }
    return 1;
}
//客户出列
Client dequeue(Window *w){
    Client c = w->data[w->front];
    w->front = (w->front + 1) % MAX_QUEUE;
    w->count--;
    return c;
}

void generate_clients(Client clients[], int *total_clients){
    int count = 0;
    int arrival = START_TIME;
    while(arrival < STOP_ARRIVE && count < 200){
        int gap = rand() % 6;
        arrival += gap;
        if(arrival >= STOP_ARRIVE){
            break;
        }
        clients[count].id = 1001 + count;
        clients[count].arrive_time = arrival;
        clients[count].duration = 1 + rand() % 10;
        clients[count].start_serve = 0;
        clients[count].wait_time = 0;
        count++;
        arrival++;
    }
    qsort(clients, count, sizeof(Client), cmp_client_arrive);
    *total_clients = count;
}

//1.新客户分配进队
int assignClientToWindow(Window windows[], int num_windows, Client c, int current_time){
    if(current_time >= STOP_ARRIVE){
        printf("[%02d:%02d] 客户#%d 来的太晚了，银行停止接单\n",current_time/60,current_time %60,c.id);
        return 0;
    }
    //找最空闲的队伍
    int min_index = 0;
    int min_count = windows[0].count;
    for(int i = 1; i < num_windows;i++){
        if(windows[i].count < min_count){
            min_count = windows[i].count;
            min_index = i;
        }
    }
    //检查最空的队是否满
    if(min_count >= MAX_QUEUE){
        printf("[%02d:%02d] 客户#%d无法取号：所有窗口均满员\n",current_time / 60,current_time %60,c.id);
        return 0;
    }
    //未满，入队
    enqueue(&windows[min_index],c);
    printf("[%02d:%02d] 客户#%d 成功进入 窗口 %d 排队\n",current_time / 60, current_time % 60, c.id, min_index + 1);
    return 1;
}

//2.每分钟更新状态
void advanceOneMinute(Window windows[],int num_windows,int current_time){
    for(int i = 0; i < num_windows; i++){//遍历每个窗口
        if(windows[i].count > 0){//如果这个窗口的count > 0,有人办理业务
            int head = windows[i].front;//队头客人的数组下标
            windows[i].data[head].duration--;
            //过去一分钟这个客人的剩余业务耗时（duration)-1
            if(windows[i].data[head].duration==0){//检查减完后时间是否为0
                Client finished_client = dequeue(&windows[i]);//办完离开
                windows[i].total_served++;//统计
                total_served_all++;
                printf("[%02d:%02d] 客户#%d 在 窗口%d 顺利办完，离开银行\n",current_time / 60, current_time % 60, finished_client.id, i + 1);
                if(windows[i].count > 0){//判断后面是否还有人排队
                    int next_head = windows[i].front;//找到i虚拟的队头
                    windows[i].data[next_head].start_serve = current_time;//记录办理时间
                    //计算等了多久
                    windows[i].data[next_head].wait_time = current_time - windows[i].data[next_head].arrive_time;
                    //业绩统计
                    total_wait_time += windows[i].data[next_head].wait_time;
                    //打印叫号
                    printf("[%02d:%02d] 叫号：请 客户#%d 到 窗口 %d 办理业务，他等了 %d 分钟\n",current_time / 60, current_time % 60, windows[i].data[next_head].id, i + 1, windows[i].data[next_head].wait_time);
                }
            }
        }
        //窗口刚进第一个人，还没开始办理
        if(windows[i].count == 1 && windows[i].data[windows[i].front].start_serve == 0){
            int head = windows[i].front;
            windows[i].data[head].start_serve = current_time;
            windows[i].data[head].wait_time = current_time - windows[i].data[head].arrive_time; // 等待时间是 0
            total_wait_time += windows[i].data[head].wait_time;
            
            printf("[%02d:%02d] 窗口 %d 当前空闲，新到客户#%d 直接开始办理（无需等待）\n", 
                   current_time / 60, current_time % 60, i + 1, windows[i].data[head].id);

        }
    } 
}

//3.八点关门
void forceCloseAndClear(Window windows[], int num_windows){
    //遍历窗口
    for(int i = 0; i < num_windows; i++){
        if(windows[i].count > 0){//检查柜台是否有人
            int head = windows[i].front;
            Client kicked_client = windows[i].data[head];
            printf("【窗口 %d】提示:客户#%d 业务未办完，20:00被强制请离银行\n", 
                   i + 1, kicked_client.id);
        
            //检查排队区
            while(windows[i].count > 0){
                Client waiting_client = dequeue(&windows[i]);
                if(waiting_client.start_serve == 0){
                    printf("窗口%d 排队区：客户#%d还在排队没轮到，20:00被强制带离\n", i+1,waiting_client.id);

                }
            }
        }
    }

}

void print_current_status(int current_time, Window windows[], int num_windows){
    printf("[%02d:%02d] 当前窗口状态：\n", current_time / 60, current_time % 60);
    for(int i = 0; i < num_windows; i++){
        printf("  窗口%d: 排队%d人, 已服务%d人, 最大队列%d人\n", i + 1, windows[i].count, windows[i].total_served, windows[i].max_len);
    }
}

void save_summary_to_log(Window windows[], int num_windows, int total_served_all, float avg_wait, int global_max_len){
    FILE *fp = fopen("bank_sim_log.txt", "w");
    if(fp == NULL){
        printf("日志文件打开失败\n");
        return;
    }
    fprintf(fp, "银行排队调度模拟系统统计结果\n");
    fprintf(fp, "全天顺利服务总人数：%d 人\n", total_served_all);
    fprintf(fp, "平均等待时间：%.2f 分钟\n", avg_wait);
    fprintf(fp, "仿真过程中最大排队长度：%d\n", global_max_len);
    for(int i = 0; i < num_windows; i++){
        fprintf(fp, "窗口%d全天接待人数：%d\n", i + 1, windows[i].total_served);
    }
    fclose(fp);
}

int main(){
    int num_windows = 10;
    Window windows[10];
    for(int i = 0; i < num_windows; i++){
        initWindow(&windows[i]);
    }
    srand((unsigned)time(NULL));

    Client clients[200];
    int total_clients = 0;
    generate_clients(clients, &total_clients);

    int next_client = 0;
    for(int t = START_TIME; t < CLOSE_TIME; t++){
        while(next_client < total_clients && clients[next_client].arrive_time == t){
            assignClientToWindow(windows, num_windows, clients[next_client], t);
            next_client++;
        }
        if(t < STOP_ARRIVE){
            advanceOneMinute(windows, num_windows, t);
        }
        if((t - START_TIME) % 120 == 0){
            print_current_status(t, windows, num_windows);
        }
    }

    forceCloseAndClear(windows, num_windows);

    int global_max_len = 0;
    for(int i = 0; i < num_windows; i++){
        if(windows[i].max_len > global_max_len){
            global_max_len = windows[i].max_len;
        }
    }

    float avg_wait = 0.0f;
    if(total_served_all > 0){
        avg_wait = (float)total_wait_time / total_served_all;
    }

    printf("全天顺利服务总人数：%d 人\n", total_served_all);
    printf("全天顺利服务总等待时间：%d 分钟\n", total_wait_time);
    printf("平均等待时间：%.2f 分钟\n", avg_wait);
    printf("仿真过程中最大排队长度：%d\n", global_max_len);
    for(int i = 0; i < num_windows; i++){
        printf("窗口%d全天接待人数：%d\n", i + 1, windows[i].total_served);
    }

    save_summary_to_log(windows, num_windows, total_served_all, avg_wait, global_max_len);
    return 0;

}


// --- 同学 C 负责的函数原型 ---
void print_current_status(int current_time, Window windows[], int num_windows);
void save_summary_to_log(Window windows[], int num_windows, int total_served_all, float avg_wait, int global_max_len);

#endif