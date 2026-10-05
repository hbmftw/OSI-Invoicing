cmake_minimum_required(VERSION 3.19)
project(OSI-NewMainWindow LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(OSI-NewMainWindow
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    mainwindow.h mainwindow.cpp mainwindow.ui
    resources.qrc
    invoice_window.h invoice_window.cpp invoice_window.ui
    DatabaseManager.h DatabaseManager.cpp
    invoice.h
    DAO_Invoice.h DAO_Invoice.cpp
    DAO_Items.h DAO_Items.cpp
    Invoiceitem.h
    payment.h
    DAO_Payment.h DAO_Payment.cpp
    addInvoice.h addInvoice.cpp addInvoice.ui
    invoicecard.h invoicecard.cpp invoicecard.ui
    invoicestatusguard.h
    client.h
    DAO_Client.h DAO_Client.cpp
    addPayment.h addPayment.cpp addPayment.ui
    DAO_Tax.h DAO_Tax.cpp
    customercard.h customercard.cpp customercard.ui
    contacts_customer.h contacts_customer.cpp contacts_customer.ui
    addclient.h addclient.cpp addclient.ui

)

target_link_libraries(OSI-NewMainWindow
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
)

install(TARGETS OSI-NewMainWindow
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET OSI-NewMainWindow
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
