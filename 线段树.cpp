#include <bits/stdc++.h>
using namespace std;
#define lc p<<1
#define rc p<<1|1
#define N 500005
int n,w[N];
struct node {
	int l,r,sum;
}tr[N*4];
void build(int p,int l,int r){
	tr[p]={l,r,w[l]};
	if(l==r) return;
	int m=l+r>>1;
	build(lc,l,m);
	build(rc,m+1,r);
	tr[p].sum=tr[lc].sum+tr[rc].sum;
}
void update(int p,int x,int k){
	if(tr[p].l==x&&tr[p].r==x){
		tr[p].sum+=k;
		return;
	}
	int m=tr[p].l+tr[p].r>>1;
	if(x<=m) update(lc,x,k);
	if(x>m) update(rc,x,k);
	tr[p].sum=tr[lc].sum+tr[rc].sum;
}
int query(int p,int x,int y){
	if(x<=tr[p].l&&tr[p].r<=y) return tr[p].sum;
	int m=tr[p].l+tr[p].r>>1;
	int sum=0;
	if(x<=m) sum+=query(lc,x,y);
	if(y>m) sum+=query(rc,x,y);
	return sum;
}
int main(){
	
}

