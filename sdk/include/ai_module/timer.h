/*
 * timer.h
 *
 * Created: 2017/08/15 15:44:02
 *  Author: aks
 */ 


#ifndef TIMER_H_
#define TIMER_H_

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
typedef void (*AiMini4WdTimerCb)(void);

/*--------------------------------------------------------------------------*/
int8_t aiMini4WdTimerRegister5msCb(AiMini4WdTimerCb cb);
int8_t aiMini4WdTimerRegister100msCb(AiMini4WdTimerCb cb);



#endif /* TIMER_H_ */