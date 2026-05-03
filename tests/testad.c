int x;
char y;
double z;
double p[100];

struct S1{
	int i;
	//char i;
	double d[2];
	char x;
	};

//struct S1{};
struct S1 p1;
struct S1 vp[10];
//struct S1 vp;
//struct S1 v[];
//struct S2 a;

double sum(double x[],int n, double m){
	//int r;
	//int n;
	double r;
	int i;
	//int m;
	r=0;
	i=0;
	while(i<n){
		double n;
		n=x[i];
		r=r+n;
		i=i+1;
		}
	return r;
	}

//void sum(){}

void f(struct S1 p){
	puti(p.i);
	}
