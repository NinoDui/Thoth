#include <memory>

#include "Entity.h"

class GoogleCloudClient {
   public:
    virtual ~GoogleCloudClient() = default;

    // Polymorphic interface
    virtual std::unique_ptr<GoogleCloudResult> execute(const GoogleCloudRequest& request) = 0;
};

class GoogleTTSClient : public GoogleCloudClient {
   public:
    std::unique_ptr<GoogleCloudResult> execute(const GoogleCloudRequest& request) override;
};