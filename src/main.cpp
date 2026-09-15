#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
using Money = int64_t; // Store paise: 100 paise = Rs 1.
constexpr Money MAX_MONEY = numeric_limits<Money>::max();

class Account {
    friend class Bank;

private:
    int id;
    string name;
    Money balance;
    vector<string> history;
    mutex mt;

public:
    Account(int accountId, string accountName, Money initialBalance)
        : id(accountId), name(accountName), balance(initialBalance) {
        if (id <= 0 || name.find_first_not_of(" \t\r\n\f\v") == string::npos
            || balance < 0) {
            throw invalid_argument("Invalid account details");
        }
    }

    bool withdraw(Money amount) {
        lock_guard<mutex> lock(mt);
        if (amount <= 0 || amount > balance) return false;

        history.push_back("Withdrew: " + to_string(amount) + " paise");
        balance -= amount;
        return true;
    }

    bool deposit(Money amount) {
        lock_guard<mutex> lock(mt);
        if (amount <= 0 || amount > MAX_MONEY - balance) return false;

        history.push_back("Deposited: " + to_string(amount) + " paise");
        balance += amount;
        return true;
    }

    Money getBalance() {
        lock_guard<mutex> lock(mt);
        return balance;
    }

    vector<string> getHistory() {
        lock_guard<mutex> lock(mt);
        return history; // Return a copy so callers cannot change our history.
    }
};

class Bank {
private:
    unordered_map<int, shared_ptr<Account>> accounts;
    mutex accountsMutex;

public:
    bool createAccount(int id, string name, Money initialBalance) {
        if (id <= 0 || name.find_first_not_of(" \t\r\n\f\v") == string::npos
            || initialBalance < 0) return false;

        lock_guard<mutex> lock(accountsMutex);
        if (accounts.find(id) != accounts.end()) return false;

        accounts.emplace(id, make_shared<Account>(id, name, initialBalance));
        return true;
    }

    shared_ptr<Account> getAccount(int id) {
        lock_guard<mutex> lock(accountsMutex);
        auto it = accounts.find(id);
        return it == accounts.end() ? nullptr : it->second;
    }

    bool transfer(int fromId, int toId, Money amount) {
        if (amount <= 0 || fromId == toId) return false;

        auto fromAcc = getAccount(fromId);
        auto toAcc = getAccount(toId);
        if (!fromAcc || !toAcc) return false;

        scoped_lock lock(fromAcc->mt, toAcc->mt);
        if (amount > fromAcc->balance
            || amount > MAX_MONEY - toAcc->balance) return false;

        fromAcc->history.push_back("Transferred out: " + to_string(amount) + " paise");
        try {
            toAcc->history.push_back("Transferred in: " + to_string(amount) + " paise");
        } catch (...) {
            // If the second entry fails, undo the first. Money is unchanged.
            fromAcc->history.pop_back();
            throw;
        }

        fromAcc->balance -= amount;
        toAcc->balance += amount;
        return true;
    }

    // Only call before workers start or after ALL account operations finish.
    // Individual locks do not make this a consistent total during transfers.
    Money getTotalBankBalance() {
        lock_guard<mutex> lock(accountsMutex);
        Money total = 0;
        for (const auto& entry : accounts) {
            Money balance = entry.second->getBalance();
            if (balance > MAX_MONEY - total) {
                throw overflow_error("Total balance is too large");
            }
            total += balance;
        }
        return total;
    }
};

void stressTestWorker(Bank& bank, int numTransfers, unsigned seed) {
    mt19937 gen(seed);
    uniform_int_distribution<int> accountDist(1, 5);
    uniform_int_distribution<Money> amountDist(100, 5'000); // Rs 1 to Rs 50.

    for (int i = 0; i < numTransfers; ++i) {
        int fromId = accountDist(gen);
        int toId = accountDist(gen);
        bank.transfer(fromId, toId, amountDist(gen));
    }
}

#ifndef BANK_NO_MAIN
int main() {
    Bank myBank;
    for (int i = 1; i <= 5; ++i) {
        myBank.createAccount(i, "User" + to_string(i), 100'000); // Rs 1,000.
    }

    Money initialTotal = myBank.getTotalBankBalance();
    constexpr int NUM_THREADS = 20;
    constexpr int TRANSFERS_PER_THREAD = 500;
    vector<thread> workers;
    workers.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(stressTestWorker, ref(myBank),
                             TRANSFERS_PER_THREAD, 12345U + static_cast<unsigned>(i));
    }
    for (auto& worker : workers) worker.join();

    Money finalTotal = myBank.getTotalBankBalance();
    cout << "Initial total: " << initialTotal << " paise\n";
    cout << "Final total:   " << finalTotal << " paise\n";
    cout << "Attempted transfers: " << NUM_THREADS * TRANSFERS_PER_THREAD << '\n';
    cout << (initialTotal == finalTotal
        ? "PASS: Balance conservation check.\n"
        : "FAIL: Balance conservation check.\n");

    return initialTotal == finalTotal ? 0 : 1;
}
#endif // BANK_NO_MAIN
