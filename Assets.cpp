#include "Assets.h"
#include <iostream>
#include <fstream>

Assets::Assets()
{

}

/** add amounts to wallet */
void Assets::addFunds()
{
    standardWallet.insertCurrency("BTC", 10);
    updateTotalAssets("BTC");
    standardWallet.insertCurrency("ETH", 500);
    updateTotalAssets("ETH");
    standardWallet.insertCurrency("USDT", 50000);
    updateTotalAssets("USDT");
    standardWallet.insertCurrency("DOGE", 10000000);
    updateTotalAssets("DOGE");
}

/** set standard order amount */
void Assets::setStandardOrderAmounts()
{
    // initializing standard amounts for orders
    standardOrderAmount["BTC"] = 0.10 * standardWallet.currencies["BTC"];
    standardOrderAmount["ETH"] = 0.10 * standardWallet.currencies["ETH"];
    standardOrderAmount["USDT"] = 0.10 * standardWallet.currencies["USDT"];
    standardOrderAmount["DOGE"] = 0.10 * standardWallet.currencies["DOGE"];
}

/** update the contents of the wallets after processing a sale */
void Assets::processSale(OrderBookEntry& sale)
{
    // extract currencies from product name
    std::vector<std::string> currs = CSVReader::tokenise(sale.product, '/');
    // ask
    if (sale.orderType == OrderBookType::asksale)
    {
        double outgoingAmount = sale.amount;
        double incomingAmount = sale.amount * sale.price;
        std::string outgoingCurrency = currs[0];
        std::string incomingCurrency = currs[1];

        // update assets
        standardWallet.currencies[incomingCurrency] += incomingAmount;
        reservedWallet.currencies[outgoingCurrency] -= outgoingAmount;
        updateTotalAssets(incomingCurrency);
        updateTotalAssets(outgoingCurrency);
    }
    // bid
    if (sale.orderType == OrderBookType::bidsale)
    {
        double incomingAmount = sale.amount;
        double outgoingAmount = sale.amount * sale.price;

        // amount was blocked in the reserved wallet at bid price but sale may have been processed at a lower price
        // storing the amount in order to release it from the reserved wallet
        double residualAmount = sale.amount * sale.priceDifference;

        std::string incomingCurrency = currs[0];
        std::string outgoingCurrency = currs[1];
        
        // update assets
        standardWallet.currencies[incomingCurrency] += incomingAmount;
        standardWallet.currencies[outgoingCurrency] += residualAmount;
        reservedWallet.currencies[outgoingCurrency] -= (outgoingAmount + residualAmount);
        updateTotalAssets(incomingCurrency);
        updateTotalAssets(outgoingCurrency);
    } 
}

/** move amounts from standard wallet to the reserved wallet */
void Assets::blockAmount(std::string currency, double amount)
{
    // move order amount for product to the reserved wallet to avoid placing uncovered bids/asks
    reservedWallet.insertCurrency(currency, amount);
    standardWallet.removeCurrency(currency, amount);
}

/** move amounts from reserved wallet back to the standard wallet */
void Assets::unblockAmount(std::string currency, double amount)
{
    // move order amount for product to the reserved wallet to avoid placing uncovered bids/asks
    standardWallet.insertCurrency(currency, amount);
    reservedWallet.removeCurrency(currency, amount);
}

/** update total assets: standard wallet + reserved wallet */
void Assets::updateTotalAssets(std::string currency)
{
    totalAssets.currencies[currency] = standardWallet.currencies[currency] + reservedWallet.currencies[currency];
}

/** update the contents of the wallets following order cancellation */
void Assets::processOrderCancellation(OrderBookEntry& order, Wallet& stdWallet, Wallet& resWallet)
{
    std::vector<std::string> currs = CSVReader::tokenise(order.product, '/');
    std::string currency;
    double blockedAmount;

    // bids block amounts in the second currency of the product pair
    if (order.orderType == OrderBookType::bid)
    {
        currency = currs[0];
        blockedAmount = order.amount * order.price;
    } 
    // asks block amounts in the first currency of the product pair
    if (order.orderType == OrderBookType::ask)
    {
        currency = currs[1];
        blockedAmount = order.amount;
    }

    // remove blocked amount from the reserved wallet and add it back to the standard wallet
    resWallet.currencies[currency] -= blockedAmount;
    stdWallet.currencies[currency] += blockedAmount;
}

/** function to log assets in all wallets (standard, reserved, total) */
void Assets::logAssets(std::ofstream& logFile)
{
    logFile << "Standard Wallet: " << std::endl;
    logFile << standardWallet.toString() << std::endl;
    logFile << "Reserved Wallet: " << std::endl;
    logFile << reservedWallet.toString() << std::endl;
    logFile << "Total Assets: " << std::endl;
    logFile << totalAssets.toString() << std::endl;
}