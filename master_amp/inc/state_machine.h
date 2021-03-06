#ifndef __FSM_H
#define __FSM_H

//extern struct transition state_transitions[];

enum state_codes { start, vol, bal, fan};
enum ret_codes { ok, fail, repeat};

int start_state(void);
int vol_state(void);
int bal_state(void);
int fan_state(void);

struct transition {
	enum state_codes src_state;
	enum ret_codes   ret_code;
	enum state_codes dst_state;
};

	
enum state_codes lookup_transitions(enum state_codes current, enum ret_codes ret);


#endif
