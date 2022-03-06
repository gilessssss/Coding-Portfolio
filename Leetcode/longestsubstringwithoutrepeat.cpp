class Solution {
public:
    int lengthOfLongestSubstring(string s){
        int frequency[256] = {0};
        int r = 0;
        int l = 0;
        int ans = 0;

        while (r < s.length()) {
            frequency[s[r]] ++;

            while (frequency[s[r]] > 1) {
                frequency[s[l]] --;
                l ++;
            }

            ans = max(ans, r - l + 1);

            r ++;
        }
        return ans;
    }
};
