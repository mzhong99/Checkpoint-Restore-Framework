
long long int set_x_to_10(int *x)
{
    *x = 10;
    return *x;
}

int set_x_to_val(int *x, int val)
{
    *x = val;
    return *x;
}

int main()
{
    int data;
    int *x = &data;
    int ret;
        
    ret = set_x_to_10(x);
    ret = set_x_to_val(x, 0xDEADBEEF);
}
