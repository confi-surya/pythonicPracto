import unittest
import sys
import os


from osd.accountService.account_service_interface import AccountServiceInterface

account_path = os.getcwd()

class TestAccountServiceInterface(unittest.TestCase):

    def setUp(self):
        pass


    def test_create_account(self):
        info = {
            'account' : "test_account",
            'created_at': 1,
            'put_timestamp': 2,
            'delete_timestamp': 3,
            'container_count': 0,
            'object_count': 0,
            'bytes_used': 0,
            'hash': '00000000000000000000000000000000',
            'id': '420',
            'status': '',
            'status_changed_at': 0,
            'metadata': ''
            }
	account_object = AccountServiceInterface()
        resp = account_object.create_account(account_path, account_object.create_AccountStat_object(info))
        self.assert_(resp is not None)


    def test_list_account(self):
        start = 0
        account_listing_limit = 1024
        account_object = AccountServiceInterface()
#        resp = account_object.list_account(account_path, START, ACCOUNT_LISTING_LIMIT)
#        self.assert_(resp is not None)

    def test_update_container(self):
        container_name = 'test_container'
        container_data = None
        account_object = AccountServiceInterface()
#        resp = account_object.update_container(account_path, container_name, container_data)
#        self.assert_(resp is not None)

    def test_get_account_stat(self):
        account_object = AccountServiceInterface()
        resp = account_object.get_account_stat(account_path)
        self.assert_(resp is not None)

    def test_set_account_stat(self):
        info = {
            'account' : "test_account",
            'created_at': 1,
            'put_timestamp': 2,
            'delete_timestamp': 3,
            'container_count': 0,
            'object_count': 0,
            'bytes_used': 0,
            'hash': '00000000000000000000000000000000',
            'id': '420',
            'status': '',
            'status_changed_at': 0,
            'metadata': 'meta Testing'
            }
        account_object = AccountServiceInterface()
        resp = account_object.set_account_stat(account_path, account_object.create_AccountStat_object(info))
        self.assert_(resp is not None)

    def test_delete_account(self):
        account_object = AccountServiceInterface()
        resp = account_object.delete_account(account_path)
        self.assert_(resp is not None)


if __name__ == '__main__':
    unittest.main()

