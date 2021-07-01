#pragma once
#include <string>
	
enum class OrderBookType {bid, ask, unknown, asksale, bidsale};

class OrderBookEntry 
{
	public:

		OrderBookEntry(double _price, 
					   double _amount, 
					   std::string _timestamp, 
					   std::string _product, 
					   OrderBookType _orderType,
					   double _priceDifference = 0,
					   std::string _username = "dataset",
					   std::string _orderStatus = "initial");

		static OrderBookType stringToOrderBookType(std::string s);

		static bool compareByTimestamp(const OrderBookEntry& e1, const OrderBookEntry& e2);

		static bool compareByPriceAsc(const OrderBookEntry& e1, const OrderBookEntry& e2);

		static bool compareByPriceDesc(const OrderBookEntry& e1, const OrderBookEntry& e2);

		double price;
		double amount;
		std::string timestamp;
		std::string product;
		OrderBookType orderType;
		double priceDifference;
		std::string username;
		std::string orderStatus;
};