#include "OrderBookEntry.h" 

OrderBookEntry::OrderBookEntry(double _price, 
                               double _amount, 
                               std::string _timestamp, 
                               std::string _product, 
                               OrderBookType _orderType,
                               double _priceDifference,
                               std::string _username,
                               std::string _orderStatus)
: price(_price), 
  amount(_amount), 
  timestamp(_timestamp), 
  product(_product), 
  orderType(_orderType),
  priceDifference(_priceDifference),
  username(_username),
  orderStatus(_orderStatus)
{

};

OrderBookType OrderBookEntry::stringToOrderBookType(std::string s)
{
  if (s == "ask")
  {
    return OrderBookType::ask;
  }
  if (s == "bid")
  {
    return OrderBookType::bid;
  }
  return OrderBookType::unknown;
};

bool OrderBookEntry::compareByTimestamp(const OrderBookEntry& e1, const OrderBookEntry& e2)
{
  return e1.timestamp < e2.timestamp;
}

bool OrderBookEntry::compareByPriceAsc(const OrderBookEntry& e1, const OrderBookEntry& e2)
{
  return e1.price < e2.price;
}

bool OrderBookEntry::compareByPriceDesc(const OrderBookEntry& e1, const OrderBookEntry& e2)
{
  return e1.price > e2.price;
}