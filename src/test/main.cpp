#include <iostream>
#include <vector>
using namespace std;

#include <vector>
using namespace std;

#include <vector>
using namespace std;

class Solution {
public:
    bool canFinish(int numCourses, vector<vector<int>>& prerequisites) {
        // 初始化图和颜色数组
        graph.assign(numCourses, vector<int>());
        colors = vector<int> (numCourses);
        // 构建图：每门课程的先修关系
        for (auto& p : prerequisites) {
            graph[p[1]].push_back(p[0]);
        }
        
        // 对每个课程进行 DFS 检测环
        for (int i = 0; i < numCourses; i++) {
            if (colors[i] == 0 && dfs(i)) {
                return false; // 检测到环，不能完成所有课程
            }
        }
        return true;
    }

private:
    // 私有成员变量：图和颜色数组
    vector<vector<int>> graph;
    vector<int> colors;  // 0: 未访问, 1: 正在访问, 2: 已访问完毕

    // 私有成员函数：DFS 检测环
    bool dfs(int course) {
        colors[course] = 1; // 标记当前课程为正在访问
        for (int next : graph[course]) {
            if (colors[next] == 1 || (colors[next] == 0 && dfs(next))) {
                return true; // 找到环
            }
        }
        colors[course] = 2; // 标记当前课程访问完毕
        return false;
    }
};



int main() {
    Solution sol;
    
    // 测试案例 1：没有环
    // 2门课程，prerequisites 表示：课程 1 依赖课程 0
    vector<vector<int>> prerequisites1 = {{1, 0}};
    cout << "Test Case 1: " << (sol.canFinish(2, prerequisites1) ? "true" : "false") << endl;
    // 预期输出：true
    
    // 测试案例 2：存在环
    // 2门课程，课程 0 和 1 互相依赖，构成环
    vector<vector<int>> prerequisites2 = {{0,1}, {1,0}};
    cout << "Test Case 2: " << (sol.canFinish(2, prerequisites2) ? "true" : "false") << endl;
    // 预期输出：false
    
    // 测试案例 3：没有任何先修课程
    vector<vector<int>> prerequisites3 = {};
    cout << "Test Case 3: " << (sol.canFinish(3, prerequisites3) ? "true" : "false") << endl;
    // 预期输出：true
    
    return 0;
}
