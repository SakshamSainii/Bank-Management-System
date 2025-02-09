#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
    #include <windows.h>
    #define CLEAR_SCREEN "cls"
    #define SLEEP(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define CLEAR_SCREEN "clear"
    #define SLEEP(ms) usleep(ms * 1000)
#endif

// Visual and branding defines
#define BANK_NAME "ROYAL CROWN BANK"
#define BANK_SLOGAN "Serving Since 1995"
#define BANK_BORDER_COLOR "\033[1;36m"  // Cyan color
#define TEXT_COLOR "\033[1;37m"         // White color
#define RESET_COLOR "\033[0m"

// File paths for data storage
#define ADMIN_TABLE "database/admin_table.txt"
#define ACCOUNTS_TABLE "database/accounts_table.txt"
#define TRANSACTIONS_TABLE "database/transactions_table.txt"
#define LOGS_TABLE "database/system_logs.txt"

// Maximum limits
#define MAX_ACCOUNTS 1000
#define MAX_TRANSACTIONS 5000
#define MAX_LINE 1024

// Structure definitions
typedef struct {
    char username[50];
    char password[50];
    int access_level;  // 1: Regular admin, 2: Super admin
} Admin;

typedef struct {
    char accountNumber[20];
    char name[100];
    char password[50];
    char phone[15];
    char email[100];
    char address[200];
    double balance;
    char accountType[20];
    char createdAt[30];
    int status;  // 0: Inactive, 1: Active, 2: Blocked
    int loginAttempts;
} Account;

typedef struct {
    char accountNumber[20];
    char type[20];
    double amount;
    double balanceAfter;
    char description[100];
    char timestamp[30];
} Transaction;

// Global variables
Admin currentAdmin;
char ERROR_MESSAGE[100];

// Visual elements functions
void loadingAnimation() {
    const char* frames[] = {
        "[█      ]", "[ ██    ]", "[  ███  ]", "[   ████ ]", "[    ████]",
        "[     ██]", "[    ███]", "[   ████]", "[  ███  ]", "[ ██    ]"
    };
    
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
            system(CLEAR_SCREEN);
            printf("\n\n\n");
            printf("                \033[1;34mLoading %s\033[0m", frames[j]);
            fflush(stdout);
            SLEEP(100);
        }
    }
}

void displayBankHeader(const char* subtitle) {
    system(CLEAR_SCREEN);
    printf("%s", BANK_BORDER_COLOR);
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    \033[1;33m%-20s\033[0m                     ║\n", BANK_NAME);
    printf("║                    \033[1;36m%-20s\033[0m                     ║\n", BANK_SLOGAN);
    printf("║                                                              ║\n");
    if (subtitle && strlen(subtitle) > 0) {
        printf("║                    \033[1;32m%-20s\033[0m                     ║\n", subtitle);
        printf("║                                                              ║\n");
    }
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", RESET_COLOR);
}

void displaySuccessMessage(const char* message) {
    printf("\n\033[1;32m✔ %s\033[0m\n", message);
    SLEEP(1500);
}

void displayErrorMessage(const char* message) {
    printf("\n\033[1;31m✖ %s\033[0m\n", message);
    SLEEP(1500);
}

// File handling functions
void createDataDirectory() {
    #ifdef _WIN32
        system("mkdir database 2> nul");
    #else
        system("mkdir -p database");
    #endif
}

void setError(const char* message) {
    strcpy(ERROR_MESSAGE, message);
}

int saveAdminToTable(Admin admin) {
    FILE* file = fopen(ADMIN_TABLE, "a");
    if (!file) {
        setError("Could not open admin table file");
        return 0;
    }

    fprintf(file, "%s|%s|%d\n", 
            admin.username, 
            admin.password, 
            admin.access_level);
    
    fclose(file);
    return 1;
}

int saveAccountToTable(Account account) {
    FILE* file = fopen(ACCOUNTS_TABLE, "a");
    if (!file) {
        setError("Could not open accounts table file");
        return 0;
    }

    fprintf(file, "%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%d|%d\n",
            account.accountNumber,
            account.name,
            account.password,
            account.phone,
            account.email,
            account.address,
            account.balance,
            account.accountType,
            account.createdAt,
            account.status,
            account.loginAttempts);
    
    fclose(file);
    return 1;
}

int saveTransactionToTable(Transaction trans) {
    FILE* file = fopen(TRANSACTIONS_TABLE, "a");
    if (!file) {
        setError("Could not open transactions table file");
        return 0;
    }

    fprintf(file, "%s|%s|%.2f|%.2f|%s|%s\n",
            trans.accountNumber,
            trans.type,
            trans.amount,
            trans.balanceAfter,
            trans.description,
            trans.timestamp);
    
    fclose(file);
    return 1;
}

Account* readAccounts(int* count) {
    FILE* file = fopen(ACCOUNTS_TABLE, "r");
    if (!file) {
        *count = 0;
        return NULL;
    }

    Account* accounts = malloc(MAX_ACCOUNTS * sizeof(Account));
    *count = 0;
    char line[MAX_LINE];

    while (fgets(line, MAX_LINE, file) && *count < MAX_ACCOUNTS) {
        Account acc;
        sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%lf|%[^|]|%[^|]|%d|%d",
               acc.accountNumber,
               acc.name,
               acc.password,
               acc.phone,
               acc.email,
               acc.address,
               &acc.balance,
               acc.accountType,
               acc.createdAt,
               &acc.status,
               &acc.loginAttempts);
        accounts[*count] = acc;
        (*count)++;
    }

    fclose(file);
    return accounts;
}

int updateAccount(Account account) {
    int count;
    Account* accounts = readAccounts(&count);
    if (!accounts) return 0;

    FILE* file = fopen(ACCOUNTS_TABLE, "w");
    if (!file) {
        free(accounts);
        return 0;
    }

    int updated = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(accounts[i].accountNumber, account.accountNumber) == 0) {
            accounts[i] = account;
            updated = 1;
        }
        fprintf(file, "%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%d|%d\n",
                accounts[i].accountNumber,
                accounts[i].name,
                accounts[i].password,
                accounts[i].phone,
                accounts[i].email,
                accounts[i].address,
                accounts[i].balance,
                accounts[i].accountType,
                accounts[i].createdAt,
                accounts[i].status,
                accounts[i].loginAttempts);
    }

    fclose(file);
    free(accounts);
    return updated;
}

Account* findAccount(const char* accountNumber) {
    int count;
    Account* accounts = readAccounts(&count);
    if (!accounts) return NULL;

    Account* found = NULL;
    for (int i = 0; i < count; i++) {
        if (strcmp(accounts[i].accountNumber, accountNumber) == 0) {
            found = malloc(sizeof(Account));
            *found = accounts[i];
            break;
        }
    }

    free(accounts);
    return found;
}

void createAccount() {
    displayBankHeader("New Account Registration");
    loadingAnimation();
    
    Account acc;
    printf("\n%sNEW ACCOUNT REGISTRATION%s\n", TEXT_COLOR, RESET_COLOR);
    printf("═══════════════════════════\n\n");

    printf("Account Number (10 digits): ");
    scanf("%s", acc.accountNumber);

    if (findAccount(acc.accountNumber)) {
        displayErrorMessage("Account number already exists!");
        return;
    }

    printf("Full Name: ");
    scanf(" %[^\n]s", acc.name);
    printf("Password: ");
    scanf("%s", acc.password);
    printf("Phone: ");
    scanf("%s", acc.phone);
    printf("Email: ");
    scanf("%s", acc.email);
    printf("Address: ");
    scanf(" %[^\n]s", acc.address);
    printf("Initial Balance: ");
    scanf("%lf", &acc.balance);
    printf("Account Type (Savings/Current): ");
    scanf("%s", acc.accountType);

    time_t now;
    struct tm* timeinfo;
    time(&now);
    timeinfo = localtime(&now);
    strftime(acc.createdAt, 30, "%Y-%m-%d %H:%M:%S", timeinfo);

    acc.status = 1;
    acc.loginAttempts = 0;

    if (saveAccountToTable(acc)) {
        Transaction trans;
        strcpy(trans.accountNumber, acc.accountNumber);
        strcpy(trans.type, "INITIAL_DEPOSIT");
        trans.amount = acc.balance;
        trans.balanceAfter = acc.balance;
        strcpy(trans.description, "Account opening deposit");
        strcpy(trans.timestamp, acc.createdAt);
        
        if (saveTransactionToTable(trans)) {
            displaySuccessMessage("Account created successfully!");
        }
    } else {
        displayErrorMessage(ERROR_MESSAGE);
    }
}

void processTransaction(const char* accountNumber, const char* type) {
    displayBankHeader("Process Transaction");
    
    Account* acc = findAccount(accountNumber);
    if (!acc) {
        displayErrorMessage("Account not found!");
        return;
    }

    double amount;
    char description[100];
    printf("Enter amount: ");
    scanf("%lf", &amount);
    printf("Enter description: ");
    scanf(" %[^\n]s", description);

    if (strcmp(type, "WITHDRAWAL") == 0) {
        if (amount > acc->balance) {
            displayErrorMessage("Insufficient balance!");
            free(acc);
            return;
        }
        acc->balance -= amount;
    } else {
        acc->balance += amount;
    }

    if (updateAccount(*acc)) {
        Transaction trans;
        strcpy(trans.accountNumber, accountNumber);
        strcpy(trans.type, type);
        trans.amount = amount;
        trans.balanceAfter = acc->balance;
        strcpy(trans.description, description);
        
        time_t now;
        struct tm* timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        strftime(trans.timestamp, 30, "%Y-%m-%d %H:%M:%S", timeinfo);

        if (saveTransactionToTable(trans)) {
            displaySuccessMessage("Transaction completed successfully!");
            printf("\nNew balance: $%.2f\n", acc->balance);
        } else {
            displayErrorMessage(ERROR_MESSAGE);
        }
    } else {
        displayErrorMessage("Could not update account!");
    }

    free(acc);
}

void displayTransactions(const char* accountNumber) {
    displayBankHeader("Transaction History");
    loadingAnimation();
    
    FILE* file = fopen(TRANSACTIONS_TABLE, "r");
    if (!file) {
        displayErrorMessage("No transactions found.");
        return;
    }

    printf("\n%sTRANSACTION HISTORY%s\n", TEXT_COLOR, RESET_COLOR);
    printf("═══════════════════\n\n");
    printf("Account: %s\n\n", accountNumber);

    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, MAX_LINE, file)) {
        Transaction trans;
        sscanf(line, "%[^|]|%[^|]|%lf|%lf|%[^|]|%[^\n]",
               trans.accountNumber,
               trans.type,
               &trans.amount,
               &trans.balanceAfter,
               trans.description,
               trans.timestamp);

        if (strcmp(trans.accountNumber, accountNumber) == 0) {
            found = 1;
            printf("Time: %s\n", trans.timestamp);
            printf("Type: %s\n", trans.type);
            printf("Amount: $%.2f\n", trans.amount);
            printf("Balance: $%.2f\n", trans.balanceAfter);
            printf("Description: %s\n", trans.description);
            printf("----------------------------------------\n");
        }
    }

    if (!found) {
        displayErrorMessage("No transactions found for this account.");
    }

    fclose(file);
}

void generateReport() {
    displayBankHeader("System Report");
    loadingAnimation();
    
    int count;
    Account* accounts = readAccounts(&count);
    if (!accounts) {
        displayErrorMessage("Could not read accounts data!");
        return;
    }

    double totalBalance = 0;
    int activeAccounts = 0;
    int savingsAccounts = 0;
    int currentAccounts = 0;

    for (int i = 0; i < count; i++) {
        totalBalance += accounts[i].balance;
        if (accounts[i].status == 1) activeAccounts++;
        if (strcmp(accounts[i].accountType, "Savings") == 0) savingsAccounts++;
        else if (strcmp(accounts[i].accountType, "Current") == 0) currentAccounts++;
    }

    printf("\n%sSYSTEM REPORT%s\n", TEXT_COLOR, RESET_COLOR);
    printf("═════════════\n\n");
    printf("Total Accounts: %d\n", count);
    printf("Active Accounts: %d\n", activeAccounts);
    printf("Savings Accounts: %d\n", savingsAccounts);
    printf("Current Accounts: %d\n", currentAccounts);
    printf("Total Balance: $%.2f\n", totalBalance);
    printf("Average Balance: $%.2f\n", count > 0 ? totalBalance/count : 0);
     free(accounts);
}

int adminLogin() {
    displayBankHeader("Admin Login");
    printf("\n%sADMIN LOGIN%s\n", TEXT_COLOR, RESET_COLOR);
    printf("═══════════════\n\n");
    
    char username[50], password[50];
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);

    // Check if admin file exists
    FILE* file = fopen(ADMIN_TABLE, "r");
    if (!file) {
        // Create first admin account
        if (strcmp(username, "admin") == 0 && strcmp(password, "admin123") == 0) {
            createDataDirectory();
            Admin admin;
            strcpy(admin.username, username);
            strcpy(admin.password, password);
            admin.access_level = 2;
            
            if (saveAdminToTable(admin)) {
                currentAdmin = admin;
                displaySuccessMessage("Admin account created successfully!");
                return 1;
            }
        }
        displayErrorMessage("Invalid credentials!");
        return 0;
    }

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, file)) {
        Admin admin;
        sscanf(line, "%[^|]|%[^|]|%d", 
               admin.username, 
               admin.password, 
               &admin.access_level);

        if (strcmp(admin.username, username) == 0 && 
            strcmp(admin.password, password) == 0) {
            currentAdmin = admin;
            fclose(file);
            displaySuccessMessage("Login successful!");
            return 1;
        }
    }

    fclose(file);
    displayErrorMessage("Invalid credentials!");
    return 0;
}

void displayMainMenu() {
    displayBankHeader("Main Menu");
    printf("\n%sMAIN MENU OPTIONS%s\n", TEXT_COLOR, RESET_COLOR);
    printf("═════════════════\n\n");
    printf("1. Create New Account\n");
    printf("2. Process Deposit\n");
    printf("3. Process Withdrawal\n");
    printf("4. View Account Balance\n");
    printf("5. View Transaction History\n");
    printf("6. Generate Report\n");
    printf("7. Exit\n\n");
    printf("Choose an option: ");
}

int main() {
    loadingAnimation();
    
    if (!adminLogin()) {
        return 1;
    }

    while (1) {
        displayMainMenu();

        int choice;
        scanf("%d", &choice);

        char accountNumber[20];

        switch (choice) {
            case 1:
                createAccount();
                break;

            case 2:
                displayBankHeader("Process Deposit");
                printf("Enter account number: ");
                scanf("%s", accountNumber);
                processTransaction(accountNumber, "DEPOSIT");
                break;

            case 3:
                displayBankHeader("Process Withdrawal");
                printf("Enter account number: ");
                scanf("%s", accountNumber);
                processTransaction(accountNumber, "WITHDRAWAL");
                break;

            case 4:
                displayBankHeader("Check Balance");
                printf("Enter account number: ");
                scanf("%s", accountNumber);
                Account* acc = findAccount(accountNumber);
                if (acc) {
                    printf("\n%sCurrent balance: $%.2f%s\n", 
                           "\033[1;32m", acc->balance, RESET_COLOR);
                    free(acc);
                } else {
                    displayErrorMessage("Account not found!");
                }
                break;

            case 5:
                printf("Enter account number: ");
                scanf("%s", accountNumber);
                displayTransactions(accountNumber);
                break;

            case 6:
                generateReport();
                break;

            case 7:
                displayBankHeader("Goodbye!");
                printf("\n%sThank you for using %s!%s\n", 
                       TEXT_COLOR, BANK_NAME, RESET_COLOR);
                SLEEP(1500);
                return 0;

            default:
                displayErrorMessage("Invalid option!");
        }

        printf("\nPress Enter to continue...");
        getchar();
        getchar();
    }

    return 0;
}