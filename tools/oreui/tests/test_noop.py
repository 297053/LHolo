import unittest


class NoopTest(unittest.TestCase):
    def test_noop(self):
        self.assertTrue(True)


if __name__ == "__main__":
    unittest.main()
