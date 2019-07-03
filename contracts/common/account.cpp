#include <contracts/account.hpp>

namespace gxc {

void account::connect(name _name, struct name service, string auth_token) {
   INLINE_ACTION_WRAPPER(account, connect, _name, (_name)(service)(auth_token));
}

void account::signin(name _name, struct name service, string auth_token) {
   INLINE_ACTION_WRAPPER(account, signin, _name, (_name)(service)(auth_token));
}

void account::setnick(name _name, string nickname) {
   INLINE_ACTION_WRAPPER(account, setnick, _name, (_name)(nickname));
}

void account::setpartner(name _name, bool is_partner) {
   INLINE_ACTION_WRAPPER(account, setpartner, default_account, (_name)(is_partner));
}

}
