use anyhow::Result;
//use serde::Serialize;
use serde_json::Value;
use ws_sdk::log::log_info;
use ws_sdk::stream::*;
use ws_sdk::crypto::secp256k1;
use ws_sdk::database::kv::*;

include!("protos/mod.rs"); 
use upload::{User_data, Payload};
use protobuf::Message;

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

static PUBK_HEX:&str = "0463FFC9FF7B8449BC8BA835C5D6D1C56AA50A4F5F71AB2D334BF8601E89F79D34420C6E6F9F3D98D8B525F05AFA5AE38EADE7CA8B7F3857CBA0AF6E9ABE6C2F4E";

fn handle(rid: i32) -> Result<()> {

    log_info(&format!("Hello WASM"))?;

    let payload = get_data(rid as _)?;
//    log_info(&format!("payload : {:?}", &payload))?;

    let upload_data = Payload::parse_from_bytes(&payload).unwrap();
    let sign : String = upload_data.sign.iter().map(|b| format!("{:02x}", b).to_string()).collect::<Vec<String>>().join("");

//    log_info(&format!("sign: {}", &sign))?;

    let user_data_type = upload_data.type_.value();
    match user_data_type {
        0 => {
            log_info(&format!("Json Format"))?;

            log_info(&format!("{:?}", &upload_data.user))?;

            let user_data = String::from_utf8(upload_data.user)?;
            log_info(&format!("{:?}", &user_data))?;

            let sensor_data: Value   = serde_json::from_str(&user_data)?;

            let sensor_1 = sensor_data["sensor_1"].as_u64().unwrap();
            let sensor_2 = sensor_data["sensor_2"].as_f64().unwrap();
            let sensor_3 = sensor_data["sensor_3"].as_bool().unwrap();

            log_info(&format!("Sensor : {} - {} - {}", sensor_1, sensor_2, sensor_3))?;

            // let msg = payload.as_str().unwrap();
            if secp256k1::verify(PUBK_HEX, user_data.as_bytes(), &sign).is_ok() {
                log_info(&format!("Success to verify user data[Json]"))?;
            } else {
                log_info(&format!("Fail to verify user data[Json]"))?;
            }
        },
        1 => {
            log_info(&format!("PB Format"))?;
            let user_data = User_data::parse_from_bytes(&upload_data.user).unwrap();
            log_info(&format!("Sensor : {:?} {:?} {:?}", &user_data.sensor_1, &user_data.sensor_2, &user_data.sensor_3))?;
        
            let msg = upload_data.user.to_vec();
            if secp256k1::verify(PUBK_HEX, &msg, &sign).is_ok() {
                log_info(&format!("Success to verify user data[PB]"))?;
            } else {
                log_info(&format!("Fail to verify user data[PB]"))?;
            }
        },
        2 => {
            log_info(&format!("Raw Format"))?;
            log_info(&format!("{:?}", &upload_data.user))?;         

            let sensor_1: u32  = convert(&upload_data.user);
            let sensor_2: f32  = convert(&&upload_data.user[4 .. ]);
            let sensor_3: bool = convert(&upload_data.user[8 .. ]);
            
            log_info(&format!("Sensor : {} - {} - {}", sensor_1, sensor_2, sensor_3))?;


            let msg = upload_data.user.to_vec();
            if secp256k1::verify(PUBK_HEX, &msg, &sign).is_ok() {
                log_info(&format!("Success to verify user data[Raw]"))?;
            } else {
                log_info(&format!("Fail to verify user data[Raw]"))?;
            }
            
        },
        _ => log_info(&format!("Format Err"))?,
    }

    Ok(())
}

fn convert<T: Copy>(pack_data: &[u8]) -> T {

    let ptr :*const u8  = pack_data.as_ptr();
    let ptr :*const T = ptr as *const T;
    let s = unsafe{ *ptr};

    s
}

fn handle_pk(rid: i32) -> Result<()> {

    let data = String::from_utf8(get_data(rid as _)?)?;
    
    let payload: Value   = serde_json::from_str(&data)?;
    let pubk = payload["pubkey"].as_str().unwrap();
    log_info(&format!("pubkey value: {}", pubk))?;

    set("pub_key", pubk.as_bytes().to_vec())?;
    log_info("success to set pubk!")?; 

    // let data_str = String::from("I got your data");
    // match mqtt::publish("topic_test", data_str.as_bytes()) {
    //     Ok(_) => log_info("publish succeeded"),
    //     _ => log_info("publish failed"),
    // };

    Ok(())
}