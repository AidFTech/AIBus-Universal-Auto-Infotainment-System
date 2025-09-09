#include "Radio_Time.h"

//Determine whether the time read through RDS is plausible.
bool getTimePlausible(int16_t h, int16_t m, const int16_t lh, const int16_t lm, const int tol) {
	if(lh < 0 && lm < 0)
		return true;
	
	bool hour_in_range = false, min_in_range = false;
	
	while(h-1 < 0)
		h += 24;
	
	while(m - tol < 0)
		m += 60;
	
	for(int i=h;i<=h+1;i+=1) {
		if(i%24 == lh) {
			hour_in_range = true;
			break;
		}
	}
	
	if(!hour_in_range) {
		for(int i=h;i>=h-1;i-=1) {
			if(i%24 == lh) {
				hour_in_range = true;
				break;
			}
		}
	}
	
	for(int i=m;i<=m+tol;i+=1) {
		if(i%60 == lm) {
			min_in_range = true;
			break;
		}
	}
	
	if(!min_in_range) {
		for(int i=m;i>=m-tol;i-=1) {
			if(i%60 == lm) {
				min_in_range = true;
				break;
			}
		}
	}
	
	return hour_in_range && min_in_range;
}