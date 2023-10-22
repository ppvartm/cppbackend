#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>
#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;
using namespace std::literals::chrono_literals;

class Order : public std::enable_shared_from_this<Order>
{
public:
    Order(net::io_context& io, Store& store, std::shared_ptr<GasCooker> cooker, HotDogHandler&& handler, int id_order, std::shared_ptr<std::mutex> m)
        :io_(io), store_(store), cooker_(cooker), handler_(std::move(handler)), id_order_(id_order), m_(m) { }
    void Execute()
    {
        m_->lock();
         sausage_ = store_.GetSausage();
         bread_ = store_.GetBread();
         m_->unlock();
        Start_Sausage_Fry();
        Start_Bread_Bake();
      //  Make_Hot_Dog();
    }

private:

    void Start_Sausage_Fry()
    {
        if (sausage_->IsCooked()) return;
        sausage_->StartFry(*cooker_, [self = shared_from_this()] {
            self->Stop_Sausage_Fry();
          /*  self->fry_sausage_.async_wait( [self = self](sys::error_code ec) {
                self->sausage_->StopFry();
                if (self->bread_->IsCooked())
                    self->handler_(HotDog(self->id_order_, self->sausage_, self->bread_));
                });
          */ 
            });
    }
    void Stop_Sausage_Fry()
    {
        fry_sausage_.expires_after(1500ms);
        fry_sausage_.async_wait( [self = shared_from_this()](sys::error_code ec) {
            if (ec) std::cout << ec.message() << "\n";
            self->m_->lock();
            self->sausage_->StopFry();
            self->is_sausage_ready_ = true;
         //   net::post(self->strand_, [self = self] {self->Make_Hot_Dog(); }); });
            if (self->is_bread_ready_ && self->is_sausage_ready_) {
                self->handler_(HotDog(self->id_order_, self->sausage_, self->bread_));  
            }
            self->m_->unlock();
            });
    }

    void Start_Bread_Bake()
    {
        if (bread_->IsCooked()) return;
        bread_->StartBake(*cooker_, [self = shared_from_this()] {
            self->Stop_Bread_Bake();
         /*   self->bake_bread_.async_wait( [self = self](sys::error_code ec) {
                self->bread_->StopBake();
                if (self->sausage_->IsCooked())
                    self->handler_(HotDog(self->id_order_, self->sausage_, self->bread_));
                });*/
            });
    }
    void Stop_Bread_Bake()
    {
        bake_bread_.expires_after(1000ms);
        bake_bread_.async_wait([self = shared_from_this()](sys::error_code ec) {
            self->m_->lock();
            self->bread_->StopBake();
            self->is_bread_ready_ = true;
            //  net::post(self->strand_, [self = self] {self->Make_Hot_Dog(); }); });
            if (self->is_bread_ready_ && self->is_sausage_ready_)
            {
            self->handler_(HotDog(self->id_order_, self->sausage_, self->bread_));
            }
            self->m_->unlock();
            });
    }
    void Make_Hot_Dog()
    {
        if(bread_->IsCooked()&&sausage_->IsCooked())
        net::post(strand_, [self=shared_from_this()] {
            self->handler_(HotDog(self->id_order_, self->sausage_, self->bread_)); });
    }

    net::io_context& io_;
    net::steady_timer fry_sausage_  { io_};
    net::steady_timer bake_bread_ { io_ };

    Store& store_;
    std::shared_ptr<GasCooker> cooker_;
    HotDogHandler handler_;

    std::shared_ptr<Sausage> sausage_ ;
    std::shared_ptr<Bread> bread_;
  
    int id_order_ = 0;
    std::shared_ptr<std::mutex> m_;
    std::atomic_bool is_bread_ready_ = false;
    std::atomic_bool is_sausage_ready_ = false;
    net::strand<net::io_context::executor_type> strand_ = net::make_strand(io_);
};


// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
     //   m.lock();
        std::make_shared<Order>(io_, store_, gas_cooker_, std::move(handler), ++i,m)->Execute();
     //   m.unlock();

     // //  m.lock();
     //   auto sausage = store_.GetSausage();
     //   auto bread = store_.GetBread();
     ////   m.unlock();
     //   net::post(io_, [handler, sausage = sausage , bread = bread, cooker = gas_cooker_, &io_ = io_, &i = i]() {
     //       sausage->StartFry(*cooker, [handler, sausage = sausage, bread = bread, &i = i] {
     //           std::this_thread::sleep_for(Milliseconds(1500));
     //           sausage->StopFry();
     //           if (bread->IsCooked())
     //               handler(HotDog(++i, sausage, bread));
     //           });
     //       });
     //   net::post(io_, [handler, sausage = sausage, bread = bread, cooker = gas_cooker_, &io_ = io_, &i = i]() {
     //       bread->StartBake(*cooker, [handler, bread = bread, sausage = sausage, &i = i] {
     //           std::this_thread::sleep_for(Milliseconds(1000)); bread->StopBake();
     //           if (sausage->IsCooked())
     //               handler(HotDog(++i, sausage, bread)); });

     //       });
    }

private:
    net::io_context& io_;
    std::atomic_int i = 0;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
};
