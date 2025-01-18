#ifndef included__jm_reactive_once_hpp
#define included__jm_reactive_once_hpp

#include <functional>

namespace jm::reactive {

template <typename T> using Consumer = std::function<void(const T &)>;
template <typename T> using Callable = std::function<T()>;

template <typename T> class Once {
   Callable<T> callable_{};
   std::vector<Consumer<T>> consumers_{};

 public:
   void subscribe(Consumer<T> &&c) {
      consumers_.push_back(std::move(c));
   }

   void subscribe() {}

   void onSuccess(Consumer<T> &&) {}

   static Once fromCallable(Callable<T> &&callable) {
      return Once{std::move(callable)};
   }

};
} // namespace jm::reactive

#endif // included__jm_reactive_once_hpp
