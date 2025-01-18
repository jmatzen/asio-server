#ifndef included__jm__builder_hpp
#define included__jm__builder_hpp


#define BUILDER_BEGIN(klass)                                                   \
   class klass##Builder {                                                      \
      klass value_;                                                            \
                                                                               \
    public:

#define BUILDER_END(klass)                                                     \
   auto &&build() { return value_; }                                           \
   }                                                                           \
   ;                                                                           \
   inline klass##Builder klass::builder() { return klass##Builder(); }

#define BUILDER_METHOD(n, t)                                                   \
   auto &n(t &&value) {                                                        \
      value_.n##_ = std::move(value);                                          \
      return *this;                                                            \
   }


#endif // included__jm__builder_hpp