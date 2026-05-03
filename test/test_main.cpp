#include <gtest/gtest.h>

#include <QCoreApplication>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    QCoreApplication app(argc, argv);

    app.setApplicationName("ThothTests");
    app.setOrganizationName("Thoth");
    return RUN_ALL_TESTS();
}
