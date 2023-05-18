use anyhow::Result;
use serde_json::Value;
use ws_sdk::log::log_info;
use ws_sdk::stream::*;
use ws_sdk::crypto::{self};
use ws_sdk::database::kv::*;

#[no_mangle]
pub extern "C" fn start_pk(rid: i32) -> i32 {
    
    log_info("hello wasm pk !").expect("err");

    match handle_pk(rid) {
        Ok(_) => return 0,
        _ => return -1,
    };

}

#[no_mangle]
pub extern "C" fn start(rid: i32) -> i32 {
    match handle(rid) {
        Ok(_) => return 0,
        _ => return -1,
    };
}

static PVK_HEX: &str = "4582b2bf2611f8fe5f7d4e22e20ff19dda42ca630344b33831695c02b616c819";
static PUBK_HEX:&str = "0463FFC9FF7B8449BC8BA835C5D6D1C56AA50A4F5F71AB2D334BF8601E89F79D34420C6E6F9F3D98D8B525F05AFA5AE38EADE7CA8B7F3857CBA0AF6E9ABE6C2F4E";

fn handle(rid: i32) -> Result<()> {

    let data = String::from_utf8(get_data(rid as _)?)?;
    log_info(&format!("payload : {}", &data))?;

    let payload: Value   = serde_json::from_str(&data)?;

    let sign = payload["sign"].as_str().unwrap();
    log_info(&format!("sign value: {}", sign))?;
  
    let message = String::from("hello devnet"); 
    let pubkey_hex = crypto::secp256k1::pubkey(PVK_HEX)?;

    // let sig_der = crypto::secp256k1::sign_der(PVK_HEX, message.as_bytes())?;
    // assert!(crypto::secp256k1::verify_der(&pubkey_hex, message.as_bytes(), &sig_der).is_ok());

    let sig = crypto::secp256k1::sign(PVK_HEX, message.as_bytes()).unwrap();
    assert!(crypto::secp256k1::verify(&pubkey_hex, message.as_bytes(), &sig).is_ok());
    log_info("demo verify pass!").unwrap();

    assert!(crypto::secp256k1::verify(PUBK_HEX, message.as_bytes(), sign).is_ok());
    log_info("dev upload verify pass!").unwrap();

    Ok(())
}

fn handle_pk(rid: i32) -> Result<()> {

    let data = String::from_utf8(get_data(rid as _)?)?;
    
    let payload: Value   = serde_json::from_str(&data)?;
    let pubk = payload["pubkey"].as_str().unwrap();
    log_info(&format!("pubkey value: {}", pubk))?;

    set("pub_key", pubk.as_bytes().to_vec())?;
    log_info("success to set pubk!")?; 

    Ok(())
}