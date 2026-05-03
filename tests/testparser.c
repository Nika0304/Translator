struct Pt{
    int x;
    int y;
};
int a;
double v[10];

struct Pt pt;

double max(double a,  double b ){
	if(a<b) return a;
		else  b;
	}

int len(char s[]){
	int i;
	i=0;
	while(s[i])i=i+1;
	return i;
	}
int foo()
{
	int a[23];
}
void main(){
	int i;
	i=10;
	while(i!=0){
		puti(i);
		i=i/2;
		}	
	}
// struct Pt{
//     int x;
//     int y;
// };

// struct Box{
//     struct Pt p;
//     double weight;
//     char label[20];
// };

// int globalA;
// double values[10];
// char text[100];
// struct Pt origin;
// struct Box box;

// double max(double a, double b){
//     if(a<b){
//         return a;
//     }else{
//         return b;
//     }
// }

// int len(char s[]){
//     int i;
//     i=0;
//     while(s[i]){
//         i=i+1;
//     }
//     return i;
// }

// int sum(int v[], int n){
//     int i;
//     int s;
//     i=0;
//     s=0;
//     while(i<n){
//         s=s+v[i];
//         i=i+1;
//     }
//     return s;
// }

// int absVal(int x){
//     if(x<0){
//         return -x;
//     }
//     return x;
// }

// int testStruct(){
//     struct Pt p;
//     p.x=10;
//     p.y=20;
//     return p.x+p.y;
// }

// int testArray(){
//     int v[5];
//     v[0]=1;
//     v[1]=2;
//     v[2]=3;
//     v[3]=4;
//     v[4]=5;
//     return v[2];
// }

// int testLogic(int a, int b, int c){
//     if(a<b && b<c || a!=0){
//         return 1;
//     }else{
//         return 0;
//     }
// }

// int foo(){
//     int a;
//     int b;
//     a=10;
//     b=20;
//     a=a+b*2;
//     b=(a+b)/(2);
//     if(a>b){
//         a=a-1;
//     }else{
//         b=b-1;
//     }
//     return a+b;
// }

// void main(){
//     int i;
//     int x;
//     i=10;
//     x=0;
//     while(i!=0){
//         x=x+i;
//         puti(i);
//         i=i/2;
//     }
//     if(x>0){
//         puti(x);
//     }else{
//         puti(0);
//     }
// }