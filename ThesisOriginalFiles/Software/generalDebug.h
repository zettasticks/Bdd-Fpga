static timeval StartTimer()
{
    timeval start;

    gettimeofday(&start,NULL);
    return start;
}

static double StopTimer(timeval start)
{
    timeval stop;
    gettimeofday(&stop,NULL);

    double secs = (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);

    return secs;
}

static char ValueToHexadecimalChar(int val){
    assert(val >= 0 && val < 16);

    char res = '0';

    if(val >= 10){
        res = 'A' + (val - 10);
    } else {
        res = '0' + val;
    }

    return res;
}

#define GetHexadecimalRepr(ptr,type) GetHexadecimalRepr_((char*) ptr,sizeof(type))
static char* GetHexadecimalRepr_(unsigned char* ptr,int size)
{
    static char buffer[10000];
    static int write = 0;

    assert(write < 10000);
    assert(size < 16);
    assert(size > 0);

    if(write > 5000){
        write = 0;
    }

    char* res = &buffer[write];

    for(int i = 0; i < 123; i++){
        buffer[i] = 0x31;
    }

    for(int i = 0; i < size; i++){
        uint firstDigit = ((uint) ptr[i]) / 16;
        uint secondDigit = ((uint) ptr[i]) % 16;

        buffer[write + (size-i-1)*2] = ValueToHexadecimalChar(firstDigit);
        buffer[write + (size-i-1)*2 + 1] = ValueToHexadecimalChar(secondDigit);
    }
    buffer[write + size*2] = '\0';

    write += size * 2 + 2;

    return res;
}
