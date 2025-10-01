#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int t;
    cin >> t;
    while(t--) {
        int l, r;
        cin >> l >> r; //here l = 0
        int n = r - l + 1;

        vector<int> a(n), b(n);
        for(int i = 0; i < n; i++) {
            b[i] = i; // b = [0..r]
        }

        //Greedy=> maximize OR by pairing numbers to get all bits set
        //Find the largest power of 2 ≤ r
        int k = 31 - __builtin_clz(r); // highest bit of r
        int M = (1 << (k + 1)) - 1; // all 1s up to highest bit

        // Initialize a with all numbers
        vector<int> unused;
        for(int i = 0; i < n; i++) unused.push_back(i);

        vector<int> ans(n);
        vector<bool> used(n, false);

        for(int i = 0; i < n; i++) {
            int target = M ^ b[i]; // number that OR with b[i] gives M
            auto it = find(unused.begin(), unused.end(), target);
            if(it != unused.end()) {
                ans[i] = target;
                used[target] = true;
                unused.erase(it);
            } else {
                // pick largest unused
                ans[i] = unused.back();
                used[unused.back()] = true;
                unused.pop_back();
            }
        }

        // Compute sum
        long long sum = 0;
        for(int i = 0; i < n; i++) {
            sum += (ans[i] | b[i]);
        }

        cout << sum << endl;
        for(int x : ans) cout << x << " ";
        cout << endl;
    }

    return 0;
}