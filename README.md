# Concurrent Bank Transfer Simulator

A small C++17 project demonstrating synchronized bank transfers. Twenty threads
attempt 10,000 transfers between five accounts, then the program checks that the
total balance is unchanged.

## Features

- Account creation, deposits, withdrawals, and transfers.
- Integer paise to avoid floating-point rounding errors.
- Per-account mutexes and deadlock-avoiding two-account locking.
- Validation for invalid amounts, duplicate IDs, insufficient funds, and overflow.
- String transaction history and separate correctness tests.

## Requirements

A C++17 compiler with thread support. The commands below use GCC on Linux.
GNU Make is optional. No third-party libraries are required.

## Build and run

Clone the repository and enter its directory:

```bash
git clone https://github.com/klauser08/concurrent-bank-simulator.git
cd concurrent-bank-simulator
```

Build and run:

```bash
make run
make test
```

Without Make:

```bash
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread src/main.cpp -o build/bank_demo
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread tests/bank_tests.cpp -o build/bank_tests
./build/bank_demo
./build/bank_tests
```

Expected demo output:

```text
Initial total: 500000 paise
Final total:   500000 paise
Attempted transfers: 10000
PASS: Balance conservation check.
```

500,000 paise equals Rs 5,000. All API amounts are integer paise: `100'000`
means Rs 1,000. Do not pass decimal rupee amounts. Attempts include rejected
self-transfers and insufficient-funds requests.

## Files

| File | Purpose |
| --- | --- |
| [src/main.cpp](src/main.cpp) | Account, Bank, worker function, and demo |
| [tests/bank_tests.cpp](tests/bank_tests.cpp) | Validation, overflow, history, and concurrency tests |
| [Makefile](Makefile) | Build, run, test, sanitizer, and cleanup commands |
| [.gitignore](.gitignore) | Keep build outputs and local files out of Git |
| README.md | Project description and instructions |

## How it works

Each account has a mutex protecting its balance and history. `lock_guard` holds
one account lock for a deposit or withdrawal. Transfers use `scoped_lock` to
acquire both account locks with deadlock avoidance. A separate mutex protects
the account registry, and `shared_ptr` keeps retrieved accounts alive.

The program joins every worker before reading the final total. Calling
`getTotalBankBalance()` is supported only before workers start or after all account
operations finish; it does not provide a consistent total during active transfers.

History entries are stored before balances change. The short transfer `try/catch`
undoes the first entry if storing the second fails. Opening balances are not logged.

## Tests and limitations

Tests cover rejected transactions, duplicate and missing accounts, exact-funds
transfers, history copies, overflow, and 16,000 opposing transfers. Checks remain
active with `-DNDEBUG`. `BANK_NO_MAIN` lets the test file include the implementation
without starting its demo; compile the test file on its own as shown above.

The demo and tests have passed locally with GCC 13.3.0 on Linux. For supported
Linux compilers, `make tsan` runs the tests with ThreadSanitizer.

This is an in-memory learning project. It has no persistence, authentication,
resource-exhaustion recovery, or durable audit log. History grows in memory.
A passing stress test does not prove every possible execution safe.
