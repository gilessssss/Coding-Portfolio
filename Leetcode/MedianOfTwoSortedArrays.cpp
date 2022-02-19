class Solution {
public:
    double findMedianSortedArrays(vector<int>& nums1, vector<int>& nums2) {
        int targetBelow;
        int m = nums1.size();
        int n = nums2.size();
        
        if (m > n) {
            return findMedianSortedArrays(nums2, nums1);
        }

        int lower = 0;
        int upper = m;

        while (lower <= upper) {
            int X = (lower + upper) / 2;
            int Y = (m + n + 1) / 2 - X;

            int max1;
            int max2;
            int min1;
            int min2;

            if (X == 0) {
                max1 = INT_MIN;
            }
            else {
                max1 = nums1[X - 1];
            }
            
            if (Y == 0) {
                max2 = INT_MIN;
            }
            else {
                max2 = nums2[Y - 1];
            }

            if (X == m) {
                min1 = INT_MAX;
            }
            else {
                min1 = nums1[X];
            }
            
            if (Y == n) {
                min2 = INT_MAX;
            }
            else {
                min2 = nums2[Y];
            }
            

            if ((max1 <= min2) && (max2 <= min1)) {
                double result;
                if ((m + n) % 2 == 0) {
                    result = (double)((max(max1, max2) + min(min2, min1))) / 2;
                    return result;
                }
                else {
                    result = max(max1, max2);
                    return result;
                }
            }
            else if (max1 > min2) {
                upper = X - 1;
            }
            else {
                lower = X + 1;
            }
        }
        return -1.0;
    }
};