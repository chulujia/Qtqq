#include "friendchatdlg.h"
#include "ui_friendchatdlg.h"

#include <QScrollBar>
#include <QTextEdit>
#include <QTextCursor>
#include <QDebug>
#include <QMouseEvent>
#include <QFileDialog>
#include <QRegExp>

#include "core/friendimgsender.h"
#include "core/qqskinengine.h"
#include "core/captchainfo.h"
#include "core/friendchatlog.h"
#include "core/qqitem.h"

FriendChatDlg::FriendChatDlg(QString uin, QString from_name, QString avatar_path, QWidget *parent) :
    QQChatDlg(uin, from_name, parent),
    ui(new Ui::FriendChatDlg()),
    avatar_path_(avatar_path)
{
   ui->setupUi(this);

   set_type(QQChatDlg::kFriend);

   initUi();
   updateSkin();
   initConnections();

   convertor_.addUinNameMap(id_, name_);
   send_url_ = "/channel/send_buddy_msg2";

   te_input_.setFocus();
}

void FriendChatDlg::initUi()
{
    setWindowTitle(name_);
    ui->lbl_name_->setText(name_);

    ui->splitter_main->insertWidget(0, &msgbrowse_);
    ui->splitter_main->setChildrenCollapsible(false);
    ui->vlo_main->insertWidget(1, &te_input_);

    ui->btn_send_key->setMenu(send_type_menu_);

    QList<int> sizes;
    sizes.append(1000);
    sizes.append(ui->splitter_main->midLineWidth());
    ui->splitter_main->setSizes(sizes);

    if (avatar_path_.isEmpty())
        avatar_path_ = QQSkinEngine::instance()->getSkinRes("default_friend_avatar");
    QFile file(avatar_path_);
    file.open(QIODevice::ReadOnly);
    QPixmap pix;
    pix.loadFromData(file.readAll());
    file.close();
    ui->lbl_avatar_->setPixmap(pix);

   resize(this->minimumSize());
}


void FriendChatDlg::initConnections()
{
    connect(ui->btn_send_img, SIGNAL(clicked(bool)), this, SLOT(openPathDialog(bool)));
    connect(ui->btn_send_msg, SIGNAL(clicked()), this, SLOT(sendMsg()));
    connect(ui->btn_qqface, SIGNAL(clicked()), this, SLOT(openQQFacePanel()));
    connect(ui->btn_close, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btn_chat_log, SIGNAL(clicked()), this, SLOT(openChatLogWin()));
}

FriendChatDlg::~FriendChatDlg()
{
    delete ui;
}

void FriendChatDlg::updateSkin()
{

}

QString FriendChatDlg::converToJson(const QString &raw_msg)
{
    QString msg_template = "r={\"to\":" + id_ +",\"face\":525,"
            "\"content\":\"[";

    //提取<p>....</p>内容
    QRegExp p_reg("(<p.*</p>)");
    p_reg.setMinimal(true);

    int pos = 0;
    while ( (pos = p_reg.indexIn(raw_msg, pos)) != -1 )
    {
        QString content = p_reg.cap(0);
        while (!content.isEmpty())
        {
            if (content[0] == '<')
            {
                int match_end_idx = content.indexOf('>')+1;
                QString single_chat_item = content.mid(0, match_end_idx);

                int img_idx = single_chat_item.indexOf("src");
                if (img_idx != -1)
                {
                    img_idx += 5;
                    int img_end_idx = content.indexOf("\"", img_idx);
                    QString src = content.mid(img_idx, img_end_idx - img_idx);

                    if (src.contains(kQQFacePre))
                    {
                        msg_template.append("[\\\"face\\\"," + src.mid(kQQFacePre.length()) + "],");
                    }
                    else
                    {
                        msg_template.append("[\\\"offpic\\\",\\\""+ id_file_hash_[src].network_path + "\\\",\\\""+ id_file_hash_[src].name + "\\\"," + QString::number(id_file_hash_[src].size) + "],");
                    }
                }

                content = content.mid(match_end_idx);
            }
            else
            {
                int idx = content.indexOf("<");
                //&符号的html表示为&amp;而在json中为%26,所以要进行转换
                QString word = content.mid(0,idx);
                jsonEncoding(word);
                msg_template.append("\\\"" + word + "\\\",");
                if (idx == -1)
                    content = "";
                else
                    content = content.mid(idx);
            }
        }

        msg_template.append("\\\"\\\\n\\\",");
        pos += p_reg.cap(0).length();
    }

    msg_template = msg_template +
            "[\\\"font\\\",{\\\"name\\\":\\\"%E5%AE%8B%E4%BD%93\\\",\\\"size\\\":\\\"10\\\",\\\"style\\\":[0,0,0],\\\"color\\\":\\\"000000\\\"}]]\","
            "\"msg_id\":" + QString::number(msg_id_++) + ",\"clientid\":\"5412354841\","
            "\"psessionid\":\""+ CaptchaInfo::singleton()->psessionid() +"\"}"
            "&clientid=5412354841&psessionid="+CaptchaInfo::singleton()->psessionid();
    //msg_template.replace("/", "%2F");
    return msg_template;
}

ImgSender* FriendChatDlg::getImgSender() const
{
    return new FriendImgSender();
}

void FriendChatDlg::getInfoById(QString id, QString &name, QString &avatar_path, bool &ok) const
{
    name = name_;
    avatar_path = avatar_path_.isEmpty() ? QQSkinEngine::instance()->getSkinRes("default_friend_avatar") : avatar_path_;
    ok = true;
}

QQChatLog *FriendChatDlg::getChatlog() const
{
    return new FriendChatLog(id());
}