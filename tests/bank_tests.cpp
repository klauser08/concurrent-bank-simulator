// Reuse the single-file implementation without its demo entry point.
#define BANK_NO_MAIN
#include "../src/main.cpp"
#include <cstdlib>

void check(bool condition, const char* expression) {
    if (!condition) {
        cerr << "FAIL: " << expression << '\n';
        exit(EXIT_FAILURE);
    }
}

// Checks stay active even when compiled with -DNDEBUG.
#define CHECK(expression) check((expression), #expression)

int main() {
    Bank bank;
    CHECK(!bank.createAccount(0, "A", 0));
    CHECK(!bank.createAccount(1, "  ", 0));
    CHECK(!bank.createAccount(1, "A", -1));
    CHECK(bank.createAccount(1, "A", 1000));
    CHECK(bank.createAccount(2, "B", 500));
    auto a = bank.getAccount(1);
    auto b = bank.getAccount(2);
    CHECK(!bank.createAccount(1, "Replace", 0));
    CHECK(bank.getAccount(1) == a);
    CHECK(!bank.getAccount(9));
    CHECK(!a->deposit(0) && !a->deposit(-1));
    CHECK(!a->withdraw(0) && !a->withdraw(-1) && !a->withdraw(1001));
    CHECK(!bank.transfer(1, 2, 0) && !bank.transfer(1, 2, -1));
    CHECK(!bank.transfer(1, 1, 1) && !bank.transfer(1, 9, 1));
    CHECK(!bank.transfer(9, 1, 1) && !bank.transfer(1, 2, 1001));
    CHECK(a->getHistory().empty() && b->getHistory().empty());
    CHECK(bank.transfer(1, 2, 1000));
    CHECK(a->getBalance() == 0 && b->getBalance() == 1500);
    CHECK(a->deposit(100) && a->withdraw(100));
    auto copy = a->getHistory();
    copy.clear();
    CHECK(a->getHistory().size() == 3);
    CHECK(bank.getTotalBankBalance() == 1500);

    Bank large;
    CHECK(large.createAccount(1, "A", MAX_MONEY));
    CHECK(large.createAccount(2, "B", 1));
    CHECK(!large.getAccount(1)->deposit(1));
    CHECK(!large.transfer(2, 1, 1));
    bool overflowCaught = false;
    try { large.getTotalBankBalance(); }
    catch (const overflow_error&) { overflowCaught = true; }
    CHECK(overflowCaught);

    Bank concurrent;
    concurrent.createAccount(1, "A", 100000);
    concurrent.createAccount(2, "B", 100000);
    vector<thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&, i] {
            for (int j = 0; j < 2000; ++j) {
                CHECK(concurrent.transfer(i % 2 + 1, 2 - i % 2, 1));
            }
        });
    }
    for (auto& t : threads) t.join();
    CHECK(concurrent.getTotalBankBalance() == 200000);
    CHECK(concurrent.getAccount(1)->getBalance() == 100000);
    CHECK(concurrent.getAccount(2)->getBalance() == 100000);
    CHECK(concurrent.getAccount(1)->getHistory().size() == 16000);
    cout << "PASS: validation, overflow, history, and opposing transfers.\n";
    return 0;
}
