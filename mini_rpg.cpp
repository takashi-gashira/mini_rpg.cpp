#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>

using namespace std;

// ==========================================
// ターミナル制御の関数
// ==========================================
struct termios original_t;
int original_flags;

void enableGameMode() {
    tcgetattr(STDIN_FILENO, &original_t);
    struct termios t = original_t;
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    original_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, original_flags | O_NONBLOCK);
}

void disableGameMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_t);
    fcntl(STDIN_FILENO, F_SETFL, original_flags);
}

// ==========================================
// キャラクタークラス（MPを廃止し、スッキリさせました）
// ==========================================
class Character {
public:
    string name;
    int hp;
    int maxHp;
    int attackPower;
    int speed;

    Character(string n, int h, int a, int s) 
        : name(n), hp(h), maxHp(h), attackPower(a), speed(s) {}

    bool isDead() { return hp <= 0; }

    void attack(Character& target) {
        cout << name << " の攻撃！" << endl;
        int range = attackPower * 0.2; 
        if (range < 1) range = 1;
        int fluctuation = (rand() % (range * 2 + 1)) - range;
        int finalDamage = attackPower + fluctuation;
        if (finalDamage < 0) finalDamage = 0;

        target.hp -= finalDamage;
        if (target.hp < 0) target.hp = 0;

        cout << target.name << " に " << finalDamage << " のダメージ！" << endl;
    }
};

bool isTeamAlive(vector<Character>& team) {
    for (int i = 0; i < team.size(); ++i) {
        if (!team[i].isDead()) return true;
    }
    return false;
}

int main() {
    srand(time(NULL)); 

    vector<Character> party;
    // HPと攻撃力だけのシンプルなステータスに変更
    party.push_back(Character("勇者", 40, 12, 5));
    party.push_back(Character("戦士", 50, 15, 4));

    vector<Character> enemies;
    enemies.push_back(Character("ゴブリンA", 20, 8, 8));
    enemies.push_back(Character("ゴブリンB", 20, 8, 7));

    cout << "モンスターの群れが現れた！！\n" << endl;

    while (isTeamAlive(party) && isTeamAlive(enemies)) {
        
        // ==========================================
        // 1. 味方パーティのターン
        // ==========================================
        for (int i = 0; i < party.size(); ++i) {
            if (party[i].isDead()) continue;
            if (!isTeamAlive(enemies)) break;

            cout << "\n【" << party[i].name << "のターン】 HP:" << party[i].hp << "/" << party[i].maxHp << endl;
            // 回復コマンドを廃止し、ダイレクトに攻撃対象を選択させる
            cout << "誰を攻撃する？" << endl;
            for (int j = 0; j < enemies.size(); ++j) {
                if (!enemies[j].isDead()) {
                    cout << j << ": " << enemies[j].name << " (HP:" << enemies[j].hp << ")" << endl;
                }
            }
            cout << "番号を入力: ";
            int targetIndex;
            cin >> targetIndex;
            cout << endl;

            party[i].attack(enemies[targetIndex]);
            cout << "--------------------------------";
        }

        cout << "\n";

        // ==========================================
        // 2. 敵パーティのターン（パリィでHP回復！）
        // ==========================================
        for (int i = 0; i < enemies.size(); ++i) {
            if (enemies[i].isDead()) continue;
            if (!isTeamAlive(party)) break;

            int targetIndex;
            do {
                targetIndex = rand() % party.size();
            } while (party[targetIndex].isDead());

            cout << "★★★ " << enemies[i].name << " が " << party[targetIndex].name << " を狙っている！ ★★★" << endl;
            cout << "      (約2秒後に攻撃到達！ タイミングよく [p] キーでパリィしろ！)" << endl;

            enableGameMode(); 

            int enemyTimer = 200; 
            bool parrySuccess = false;

            while (true) {
                char c = 0;
                read(STDIN_FILENO, &c, 1);

                if (c == 'p') {
                    if (enemyTimer > 0 && enemyTimer < 40) { 
                        parrySuccess = true;
                        break;
                    } else {
                        break; 
                    }
                }

                enemyTimer--;
                if (enemyTimer <= 0) {
                    break; 
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            disableGameMode(); 

            // ★パリィ成功でHPが回復する処理
            if (parrySuccess) {
                int healAmount = 15;
                party[targetIndex].hp += healAmount;
                if (party[targetIndex].hp > party[targetIndex].maxHp) {
                    party[targetIndex].hp = party[targetIndex].maxHp; // 最大HPを超えないように調整
                }
                cout << "\rキィン！ パリィ成功！！！ " << party[targetIndex].name << " は攻撃を弾き、HPが " << healAmount << " 回復した！\n" << endl;
            } else {
                if (enemyTimer <= 0) {
                    cout << "\rドガッ！ 攻撃が直撃！" << endl;
                } else {
                    cout << "\rパリィ失敗！ タイミングがずれた！" << endl;
                }
                enemies[i].attack(party[targetIndex]);
            }
            cout << "--------------------------------\n";
        }
    }

    cout << "\n戦闘終了！" << endl;
    if (isTeamAlive(party)) cout << "モンスターたちをやっつけた！" << endl;
    else cout << "パーティは全滅してしまった..." << endl;

    return 0;
}