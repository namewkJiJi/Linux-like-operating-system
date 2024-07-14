#include"dev/dev.h"
#include"dev/tty.h"
#include"tools/log.h"
#include"dev/kbd.h"
#include"dev/console.h"
#include "cpu/irq.h"

static tty_t tty_tbl[TTY_NR];
static int curr_tty = 0;

static void init_tty_fifo(tty_fifo_t* fifo,char * buf,int size){
    fifo->buf = buf;
    fifo->size = size;
    fifo->count = 0;
    fifo->write = fifo->read = 0;
}

int tty_fifo_put(tty_fifo_t * fifo,char c){
    irq_state_t state = irq_enter_protection();

    if(fifo->count >= fifo->size){
        //满了
        irq_leave_protection(state);
        return -1;
    }

    fifo->buf[fifo->write++] = c;
    if(fifo->write >= fifo->size){
        fifo->write = 0;
    }
    fifo->count++;


    irq_leave_protection(state);
    return 0;
}

int tty_fifo_get(tty_fifo_t * fifo,char * c){
    irq_state_t state = irq_enter_protection();
    
    if(fifo->count <= 0){
        irq_leave_protection(state);

        return -1;
    }

    *c = fifo->buf[fifo->read++];
    if(fifo->read >= fifo->size){
        fifo->read = 0;
    }
    fifo->count--;

    irq_leave_protection(state);

    return 0; 
}

static tty_t * get_tty(device_t * dev){
    int idx = dev->minor;
    if(idx < 0 || idx >= TTY_NR || (dev->open_count == 0)){
        log_printf("tty is not opened");
        return (tty_t * )0;
    }

    return tty_tbl + idx;
}   

int tty_open (device_t * dev){
    int idx = dev->minor;

    if(idx < 0 || idx >= TTY_NR){
        log_printf("tty open failed\n");
        return -1;
    }

    //init tty
    tty_t * tty = tty_tbl + idx;

    init_tty_fifo(&tty->ififo,tty->ibuf,sizeof(tty->ibuf));
    init_tty_fifo(&tty->ofifo,tty->obuf,sizeof tty->obuf);

    tty->console_idx = idx;
    sem_init(&tty->osem,TTY_OBUF_SIZE);
    sem_init(&tty->isem,0);
    tty->iflags = TTY_INCLR | TTY_IECHO;
    tty->oflags = TTY_OCRLF;
    kbd_init();
    console_init(idx);

    return 0;
}

int tty_write (device_t * dev, int addr, char * buf, int size)
{
    if(!dev || size < 0){
        return -1;
    }

    tty_t * tty = get_tty(dev);
    
    if(!tty){
        return -1;
    }

    int len = 0;
    
    int err;
    while(size){
        char c = *buf++;
        if(c == '\n' && (tty->oflags & TTY_OCRLF)){
            sem_wait(&tty->osem);
            int err  = tty_fifo_put(&tty->ofifo,'\r');
            if(err<0){
                break;
            }
        }

        sem_wait(&tty->osem);
        err = tty_fifo_put(&tty->ofifo,c);
        if(err < 0){
            //插不下了，返回-1，跳出当前循环？不应该为原地等待缓冲区不满么？
            break;
        }

        len++;
        size--;
        

        console_write(tty);
    }

    return len;
}


int tty_read(device_t * dev, int addr, char * buf, int size)
{
    if(size < 0){
        return -1;
    }

    tty_t * tty = get_tty(dev);
    char* p_buf = buf;
    int len = 0;

    while(len < size){
        sem_wait(&tty->isem);

        char ch;
        tty_fifo_get(&tty->ififo,&ch);

        switch (ch)
        {
        case ASCII_DEL:
            if (len == 0) {
                continue;
            }
            len--;
            p_buf--;
            break;
        case '\n':
            if((tty->iflags & TTY_INCLR) && (len < size -1)){
                *p_buf++ =  '\r';
                len++;
            }

            *p_buf++ = '\n';
            len++;
            break;
        
        default:
            *p_buf++ = ch;
            len++;

            break;
        }

        if(tty->iflags & TTY_IECHO){
            tty_write(dev,0,&ch,1);
        }

        if((ch == '\n') || (ch == '\r')){
            break;
        }
    }

    
    return len;
}


int tty_control(device_t * dev, int cmd, int arg0, int arg1)
{   

    tty_t * tty = get_tty(dev);
    
    switch (cmd){
        case TTY_CMD_ECHO:
            if(arg0){
                tty->iflags |= TTY_IECHO;
            }else{
                tty->iflags &= ~TTY_IECHO;
            }
            
            break;
        default:
            break;
    }
    return 0;
}

void tty_close(device_t * dev){

}

dev_desc_t dev_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close
};


void tty_in(char ch){
    int idx = curr_tty;

    tty_t * tty = tty_tbl + idx;

    if(sem_count(&tty->isem) >= TTY_IBUF_SIZE){
        return;
    }

    tty_fifo_put(&tty->ififo,ch);
    sem_notify(&tty->isem);
}

void tty_select(int tty){
    if(tty != curr_tty){
        console_select(tty);
        curr_tty = tty;
    }
}