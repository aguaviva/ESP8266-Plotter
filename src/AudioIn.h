typedef void (*GetSampleFn)();


void aiBegin(GetSampleFn pfn, unsigned int sampleRate);
void aiEnd();
bool aiIsRunning();
