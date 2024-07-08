#include <stdio.h>

#include <SDL.h>
#undef main
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)
void SDL_01(){
    SDL_Window *window = NULL;

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Basic Window",
                              50,
                              50,
                              640,
                              480,
                              SDL_WINDOW_OPENGL);
    if(!window){
        printf("Can't Create Window,err: %s\n",SDL_GetError());
    }
    SDL_Event event;
    while(1){
       if(SDL_PollEvent(&event))
       {
           if(event.type == SDL_QUIT){
               break;
           }
       }
    }
    printf("Quit Success\n");
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDL_02Event(){
    SDL_Window *window = NULL;
    SDL_Renderer *render = NULL;
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Basic Window",
                              SDL_WINDOWPOS_UNDEFINED,// x位置
                              SDL_WINDOWPOS_UNDEFINED,// y位置
                              640,
                              480,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);

    render = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(render, 255, 0, 0, 255);

    SDL_RenderClear(render);

    SDL_RenderPresent(render);

    SDL_Event event;
    int b_exit = 0;
    for(;b_exit == 0;){
        SDL_WaitEvent(&event);
        //键盘点击事件
        switch(event.type){
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_a:
                    printf("key down a\n");
                    break;
                case SDLK_s:
                    printf("key down b\n");
                case SDLK_d:
                    printf("key down d\n");
                    break;
                case SDLK_q:
                    printf("key down q and push quit event\n");
                    SDL_Event event_q;
                    event_q.type = FF_QUIT_EVENT;
                    SDL_PushEvent(&event_q);
                    break;
                default:
                    printf("Key down 0x%x\n",event.key.keysym.sym);
                    break;
                }
            break;
            //鼠标点击事件
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT){
                    printf("Mouse down left\n");
                }
                else if(event.button.button == SDL_BUTTON_RIGHT){
                    printf("Mouse down right\n");
                }
                else{
                    printf("Mouse down %d\n",event.button.button);
                }
            break;
            case SDL_MOUSEMOTION:
                printf("Mouse move (%d,%d)\n",event.button.x,event.button.y);
            break;
            case FF_QUIT_EVENT:
                printf("Receive quit event\n");
                b_exit = 1;
            break;
        }
    }
}

SDL_mutex *s_lock = NULL;
SDL_cond *s_cond = NULL;

int thread_work(void *arg){
    SDL_LockMutex(s_lock);
    printf("  <=========thread_work sleep\n");
    sleep(10);
    printf("  <=========thread_work wait\n");
    SDL_CondWait(s_cond, s_lock);
    printf("  <=========thread_work receive signal, continue to do~_~!!!\n");
    printf("  <=========thread_work end\n");
    SDL_UnlockMutex(s_lock);
    return 0;
}

void MultThread(){

    SDL_Thread* t = SDL_CreateThread(thread_work,"Thread_Work",NULL);
    if(t==NULL){
        printf("  %s\n",SDL_GetError());
    }
    for(int i = 0;i<2;i++){
        sleep(2);
        printf("Main_Thread exec\n");
    }
    printf("Main SDL_LockMutex(s_lock) before =====>\n");
    SDL_LockMutex(s_lock);
    printf("Main Ready send signal=====>\n");
    printf("Main SDL_CondSignal(s_cond)before =====>\n");
    SDL_CondSignal(s_cond);
    printf("Main SDL_CondSignal(s_cond) after=====>\n");
    sleep(10);
    SDL_UnlockMutex(s_lock);
    printf("Main SDL_UnlockMutex(s_lock) after=======>\n");

    SDL_WaitThread(t,NULL);
    SDL_DestroyMutex(s_lock);
    SDL_DestroyCond(s_cond);
}

int main()
{
    printf("Hello World!\n");
    s_lock = SDL_CreateMutex();
    s_cond = SDL_CreateCond();
    MultThread();
    printf("Function Exec Ok\n");
    return 0;
}
