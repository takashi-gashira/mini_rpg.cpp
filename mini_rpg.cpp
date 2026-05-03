#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
// ★ リアルタイム処理用のおまじないを追加
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>

using namespace std;

// ==========================================
// ★ ターミナル制御の関数（追加）
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


// キャラクターの設計図（変更なし！）
class Character {
public:
    string name;
    int hp;
    int maxHp;
    int mp;
    int maxMp;
    int attackPower;
    int speed;

    Character(string n, int h, int m, int a, int s) 
        : name(n), hp(h), maxHp(h), mp(m), maxMp(m), attackPower(a), speed(s) {}

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

    void heal() {
        int mpCost = 5;
        if (mp >= mpCost) {
            mp -= mpCost;
            int healAmount = 15;
            hp += healAmount;
            if (hp > maxHp) hp = maxHp; 
            cout << name << " は回復魔法を唱えた！ (HPが " << healAmount << " 回復)" << endl;
        } else {
            cout << name << " はMPが足りない！" << endl;
        }
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
    party.push_back(Character("勇者", 30, 10, 10, 5));
    party.push_back(Character("魔法使い", 20, 30, 4, 4));

    vector<Character> enemies;
    enemies.push_back(Character("ゴブリンA", 15, 0, 5, 8));
    enemies.push_back(Character("ゴブリンB", 15, 0, 5, 7));

    cout << "モンスターの群れが現れた！！\n" << endl;

    while (isTeamAlive(party) && isTeamAlive(enemies)) {
        
        // ==========================================
        // 1. 味方パーティのターン（通常のコマンド入力）
        // ==========================================
        for (int i = 0; i < party.size(); ++i) {
            if (party[i].isDead()) continue;
            if (!isTeamAlive(enemies)) break;

            cout << "\n【" << party[i].name << "のターン】 HP:" << party[i].hp << " MP:" << party[i].mp << endl;
            cout << "どうする？ (1: たたかう, 2: かいふく): ";
            int command;
            cin >> command;

            if (command == 1) {
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
            } 
            else if (command == 2) {
                party[i].heal();
            }
            cout << "--------------------------------";
        }

        cout << "\n";

        // ==========================================
        // 2. 敵パーティのターン（★ここにパリィを組み込む！）
        // ==========================================
        for (int i = 0; i < enemies.size(); ++i) {
            if (enemies[i].isDead()) continue;
            if (!isTeamAlive(party)) break;

            int targetIndex;
            do {
                targetIndex = rand() % party.size();
            } while (party[targetIndex].isDead());

            // ★パリィの演出とリアルタイム処理開始
            cout << "★★★ " << enemies[i].name << " が " << party[targetIndex].name << " を狙っている！ ★★★" << endl;
            cout << "      (約2秒後に攻撃到達！ タイミングよく [p] キーでパリィしろ！)" << endl;

            enableGameMode(); // ここでOSの壁を解除！止まらないループへ！

            int enemyTimer = 200; // 約2秒
            bool parrySuccess = false;

            while (true) {
                char c = 0;
                read(STDIN_FILENO, &c, 1);

                if (c == 'p') {
                    if (enemyTimer > 0 && enemyTimer < 40) { // 受付時間は最後の0.4秒間
                        parrySuccess = true;
                        break;
                    } else {
                        break; // 失敗（早すぎた）
                    }
                }

                enemyTimer--;
                if (enemyTimer <= 0) {
                    break; // 時間切れ（直撃）
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            disableGameMode(); // ★絶対に通常の入力モードに戻す！

            // ★パリィの結果に応じて処理を分岐
            if (parrySuccess) {
                cout << "\rキィン！ パリィ成功！！！ " << party[targetIndex].name << " は攻撃を弾いた！ ノーダメージ！\n" << endl;
            } else {
                if (enemyTimer <= 0) {
                    cout << "\rドガッ！ 攻撃が直撃！" << endl;
                } else {
                    cout << "\rパリィ失敗！ タイミングがずれた！" << endl;
                }
                // 通常のダメージ処理を呼び出す
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