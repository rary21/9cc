int printQueen(int *queen)
{
    int i;
    
    printf("%d queen : ", 8);
    for (i = 0; i < 8; i = i+1) {
        printf("%d ", queen[i]);
    }
    printf("\n");
    return 0;
}


int check(int *queen)
{
    int i;
    int j;
    
    for (i = 0; i < 8-1; i=i+1) {
        for (j = i+1; j < 8; j=j+1) {
            if (queen[i] == queen[j]) return 0;
            if (queen[i] - queen[j] == j - i) return 0;
            if (queen[j] - queen[i] == j - i) return 0;
        }
    }
    return 1;
}


int setQueen(int *queen, int i)
{
    int j;

    if (i == 8) {
        if (check(queen)) printQueen(queen);
        return 1;
    }
    
    for (j = 0; j < 8; j=j+1) {
        queen[i] = j;
        setQueen(queen, i+1);
    }
    return 0;
}

int main()
{
    int queen[8];
    setQueen(queen, 0);
    return 0;
}