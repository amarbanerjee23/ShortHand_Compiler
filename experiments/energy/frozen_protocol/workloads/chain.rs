use std::io::Read;
fn main() {
    let mut text = String::new();
    std::io::stdin().read_to_string(&mut text).unwrap();
    let n: i32 = text.trim().parse().unwrap();
    let mut s: i32 = 7;
    for i in 0..n {
        s = (s * 31 + i) % 1000003;
    }
    println!("{}", s);
}
